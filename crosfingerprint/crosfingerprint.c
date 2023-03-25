#include "crosfingerprint.h"
#include "ec_commands.h"

#define bool int

static ULONG CrosFPDebugLevel = 0;
static ULONG CrosFPDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT  DriverObject,
	__in PUNICODE_STRING RegistryPath
)
{
	NTSTATUS               status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES  attributes;

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_INIT,
		"Driver Entry\n");

	WDF_DRIVER_CONFIG_INIT(&config, CrosFPEvtDeviceAdd);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	//
	// Create a framework driver object to represent our driver.
	//

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,
		&config,
		WDF_NO_HANDLE
	);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
			"WdfDriverCreate failed with status 0x%x\n", status);
	}

	return status;
}

BOOLEAN OnInterruptIsr(
	WDFINTERRUPT Interrupt,
	ULONG MessageID) {
	UNREFERENCED_PARAMETER(MessageID);

	WDFDEVICE Device = WdfInterruptGetDevice(Interrupt);
	PCROSFP_CONTEXT pDevice = GetDeviceContext(Device);

	struct ec_response_host_event_mask r;

	NTSTATUS status = cros_ec_command(pDevice, EC_CMD_HOST_EVENT_GET_B, 0, NULL, 0, (UINT8*)&r, sizeof(r));
	if (!NT_SUCCESS(status)) {
		goto out;
	}

	CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
		"Got event 0x%x!\n", r.mask);

	struct ec_params_host_event_mask p;
	p.mask = r.mask;

	status = cros_ec_command(pDevice, EC_CMD_HOST_EVENT_CLEAR_B, 0, (UINT8*)&p, sizeof(p), NULL, 0);
	if (!NT_SUCCESS(status)) {
		goto out;
	}

out:
	WdfInterruptQueueDpcForIsr(Interrupt);
	return true;
}

void OnInterruptDpc
(
	WDFINTERRUPT Interrupt,
	WDFOBJECT AssociatedObject
)
{
	WDFDEVICE Device = WdfInterruptGetDevice(Interrupt);
	PCROSFP_CONTEXT pDevice = GetDeviceContext(Device);
	NTSTATUS status;

	BOOLEAN ec_has_more_events = TRUE;
	while (ec_has_more_events) {
		struct ec_response_get_next_event event = { 0 };
		status = cros_ec_command(pDevice, EC_CMD_GET_NEXT_EVENT, 0, NULL, 0, (UINT8*)&event, sizeof(event));
		if (!NT_SUCCESS(status)) {
			goto out;
		}

		UINT8 event_type = event.event_type & EC_MKBP_EVENT_TYPE_MASK;
		if (event_type == EC_MKBP_EVENT_FINGERPRINT) {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
				"Fingerprint MKBP! 0x%x\n", event.data.fp_events);

			CompleteFPRequest(pDevice, event.data.fp_events);
		}
		else if (event_type == EC_MKBP_EVENT_HOST_EVENT) {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
				"Host Event MKBP %d!\n", event.data.host_event);

			if (event.data.host_event & EC_HOST_EVENT_MASK(EC_HOST_EVENT_INTERFACE_READY)) {
				CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
					"Host Event Ready!\n");
				pDevice->DeviceReady = TRUE;
			}
		}
		else {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
				"Type %d MKBP!\n", event_type);
		}

		ec_has_more_events = ((event.event_type & EC_MKBP_HAS_MORE_EVENTS) == EC_MKBP_HAS_MORE_EVENTS);
	}

out:
	return;
}

NTSTATUS
OnPrepareHardware(
	_In_  WDFDEVICE     FxDevice,
	_In_  WDFCMRESLIST  FxResourcesRaw,
	_In_  WDFCMRESLIST  FxResourcesTranslated
)
/*++

Routine Description:

This routine caches the SPB resource connection ID.

Arguments:

FxDevice - a handle to the framework device object
FxResourcesRaw - list of translated hardware resources that
the PnP manager has assigned to the device
FxResourcesTranslated - list of raw hardware resources that
the PnP manager has assigned to the device

Return Value:

Status

--*/
{
	PCROSFP_CONTEXT pDevice = GetDeviceContext(FxDevice);
	BOOLEAN fDataResourceFound = FALSE, fIRQResourceFound = FALSE;
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
	WDF_INTERRUPT_CONFIG interruptConfig;

	UNREFERENCED_PARAMETER(FxResourcesRaw);

	//
	// Parse the peripheral's resources.
	//

	ULONG resourceCount = WdfCmResourceListGetCount(FxResourcesTranslated);

	for (ULONG i = 0; i < resourceCount; i++)
	{
		PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor, pDescriptorRaw;
		UCHAR Class;
		UCHAR Type;

		pDescriptor = WdfCmResourceListGetDescriptor(
			FxResourcesTranslated, i);
		pDescriptorRaw = WdfCmResourceListGetDescriptor(
			FxResourcesRaw, i);

		switch (pDescriptor->Type)
		{
		case CmResourceTypeConnection:
			//
			// Look for I2C or SPI resource and save connection ID.
			//
			Class = pDescriptor->u.Connection.Class;
			Type = pDescriptor->u.Connection.Type;
			if (Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
				Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI)
			{
				if (fDataResourceFound == FALSE)
				{
					status = STATUS_SUCCESS;
					pDevice->SpbContext.SpiResHubId.LowPart = pDescriptor->u.Connection.IdLowPart;
					pDevice->SpbContext.SpiResHubId.HighPart = pDescriptor->u.Connection.IdHighPart;
					pDevice->IoType = CROSFP_TYPESPI;
					fDataResourceFound = TRUE;
				}
			} else if (Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
				Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_UART)
			{
				if (fDataResourceFound == FALSE)
				{
					status = STATUS_SUCCESS;
					pDevice->UartContext.UartResHubId.LowPart = pDescriptor->u.Connection.IdLowPart;
					pDevice->UartContext.UartResHubId.HighPart = pDescriptor->u.Connection.IdHighPart;
					pDevice->IoType = CROSFP_TYPEUART;
					fDataResourceFound = TRUE;
				}
			}
			break;
		case CmResourceTypeInterrupt:
			WDF_INTERRUPT_CONFIG_INIT(
				&interruptConfig,
				OnInterruptIsr,
				OnInterruptDpc);
			interruptConfig.PassiveHandling = TRUE;
			interruptConfig.InterruptRaw = pDescriptorRaw;
			interruptConfig.InterruptTranslated = pDescriptor;
			fIRQResourceFound = TRUE;

			status = WdfInterruptCreate(
				pDevice->FxDevice,
				&interruptConfig,
				WDF_NO_OBJECT_ATTRIBUTES,
				&pDevice->Interrupt);

			if (!NT_SUCCESS(status))
			{
				CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
					"Error creating WDF interrupt object - 0x%x",
					status);

				return status;
			}
			break;
		default:
			//
			// Ignoring all other resource types.
			//
			break;
		}
	}

	//
	// An SPB resource is required.
	//

	if (fDataResourceFound == FALSE || fIRQResourceFound == FALSE)
	{
		status = STATUS_NOT_FOUND;
	}

	switch (pDevice->IoType) {
	case CROSFP_TYPESPI:
		status = SpbTargetInitialize(FxDevice, &pDevice->SpbContext);
		break;
	case CROSFP_TYPEUART:
		status = UartTargetInitialize(FxDevice, &pDevice->UartContext);
		break;
	default:
		status = STATUS_INVALID_DEVICE_STATE;
		break;
	}
		
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	return status;
}

NTSTATUS
OnReleaseHardware(
	_In_  WDFDEVICE     FxDevice,
	_In_  WDFCMRESLIST  FxResourcesTranslated
)
/*++

Routine Description:

Arguments:

FxDevice - a handle to the framework device object
FxResourcesTranslated - list of raw hardware resources that
the PnP manager has assigned to the device

Return Value:

Status

--*/
{
	PCROSFP_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(FxResourcesTranslated);

	switch (pDevice->IoType) {
	case CROSFP_TYPESPI:
		SpbTargetDeinitialize(FxDevice, &pDevice->SpbContext);
		break;
	case CROSFP_TYPEUART:
		UartTargetDeinitialize(FxDevice, &pDevice->UartContext);
		break;
	default:
		status = STATUS_INVALID_DEVICE_STATE;
		break;
	}
	

	return status;
}

NTSTATUS
OnD0Entry(
	_In_  WDFDEVICE               FxDevice,
	_In_  WDF_POWER_DEVICE_STATE  FxPreviousState
)
/*++

Routine Description:

This routine allocates objects needed by the driver.

Arguments:

FxDevice - a handle to the framework device object
FxPreviousState - previous power state

Return Value:

Status

--*/
{
	UNREFERENCED_PARAMETER(FxPreviousState);

	PCROSFP_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	pDevice->DeviceCalibrated = FALSE;

	struct ec_response_get_version r;
	status = cros_ec_command(pDevice, EC_CMD_GET_VERSION, 0, NULL, 0, &r, sizeof(struct ec_response_get_version));
	if (NT_SUCCESS(status)) {
		/* Ensure versions are null-terminated before we print them */
		r.version_string_ro[sizeof(r.version_string_ro) - 1] = '\0';
		r.version_string_rw[sizeof(r.version_string_rw) - 1] = '\0';

		DebugLog("EC RO Version: %s\n", r.version_string_ro);
		DebugLog("EC RW Version: %s\n", r.version_string_rw);
	}
	else {
		DebugLog("EC Command failed with status %x\n", status);
		return status;
	}

	struct ec_response_get_protocol_info info;
	status = cros_ec_command(pDevice, EC_CMD_GET_PROTOCOL_INFO, 0, NULL, 0, &info, sizeof(info));
	if (NT_SUCCESS(status)) {
		pDevice->MaxOutsize = info.max_request_packet_size - sizeof(struct ec_host_request);
		pDevice->MaxInsize = info.max_response_packet_size - sizeof(struct ec_host_response);

		DebugLog("Max outsize: %d, Max insize: %d\n", pDevice->MaxOutsize, pDevice->MaxInsize);
	}
	else {
		DebugLog("EC Command failed with status %x\n", status);
		return status;
	}

	pDevice->DeviceReady = FALSE;
	pDevice->FingerUp = TRUE;
	pDevice->NextMode = FP_MODE_DEEPSLEEP;
	NTSTATUS status2 = cros_ec_command(pDevice, EC_CMD_REBOOT, 0, NULL, 0, NULL, 0);
	if (!NT_SUCCESS(status2)) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"Warning: Reboot failed 0x%x\n", status2);
	}

	return status;
}

NTSTATUS
OnD0Exit(
	_In_  WDFDEVICE               FxDevice,
	_In_  WDF_POWER_DEVICE_STATE  FxPreviousState
)
/*++

Routine Description:

This routine destroys objects needed by the driver.

Arguments:

FxDevice - a handle to the framework device object
FxPreviousState - previous power state

Return Value:

Status

--*/
{
	UNREFERENCED_PARAMETER(FxDevice);
	UNREFERENCED_PARAMETER(FxPreviousState);

	//PCROSFP_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	return status;
}

NTSTATUS
CrosFPEvtDeviceAdd(
	IN WDFDRIVER       Driver,
	IN PWDFDEVICE_INIT DeviceInit
)
{
	NTSTATUS                      status = STATUS_SUCCESS;
	WDF_IO_QUEUE_CONFIG           queueConfig;
	WDF_OBJECT_ATTRIBUTES         attributes;
	WDFDEVICE                     device;
	PCROSFP_CONTEXT               devContext;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_PNP,
		"CrosFPEvtDeviceAdd called\n");

	{
		WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
		WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

		pnpCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
		pnpCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
		pnpCallbacks.EvtDeviceD0Entry = OnD0Entry;
		pnpCallbacks.EvtDeviceD0Exit = OnD0Exit;

		WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);
	}

	//
	// Setup the device context
	//

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CROSFP_REQ_CONTEXT);

	WdfDeviceInitSetRequestAttributes(DeviceInit, &attributes);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CROSFP_CONTEXT);

	//
	// Create a framework device object.This call will in turn create
	// a WDM device object, attach to the lower stack, and set the
	// appropriate flags and attributes.
	//

	status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfDeviceCreate failed with status code 0x%x\n", status);

		return status;
	}

	{
		WDF_DEVICE_STATE deviceState;
		WDF_DEVICE_STATE_INIT(&deviceState);

		deviceState.NotDisableable = WdfFalse;
		WdfDeviceSetDeviceState(device, &deviceState);
	}

	//
	// Create manual I/O queue to take care of hid report read requests
	//

	devContext = GetDeviceContext(device);

	devContext->FxDevice = device;
	devContext->CurrentCapture = NULL;

	//
	// Allocate a waitlock to guard access to the default buffers
	//
	status = WdfWaitLockCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		&devContext->IoLock);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error creating Io Lock - %!STATUS!\n",
			status);
		return status;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchParallel);

	queueConfig.EvtIoDeviceControl = CrosFPEvtIoDeviceControl;

	status = WdfIoQueueCreate(device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&devContext->Queue
	);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfIoQueueCreate failed 0x%x\n", status);

		return status;
	}

	status = WdfDeviceConfigureRequestDispatching(device, devContext->Queue, WdfRequestTypeDeviceControl);
	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfDeviceConfigureRequestDispatching failed 0x%x\n", status);

		return status;
	}

	//Create WBDI Interface

	status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_BIOMETRIC_READER, NULL);
	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfDeviceCreateDeviceInterface failed 0x%x\n", status);

		return status;
	}

	WdfDeviceSetDeviceInterfaceState(device, &GUID_DEVINTERFACE_BIOMETRIC_READER, NULL, TRUE);

	CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
		"CrosFPEvtDeviceAdd succeeded 0x%x\n", status);

	return status;
}

VOID
CrosFPEvtIoDeviceControl(
	IN WDFQUEUE     Queue,
	IN WDFREQUEST   Request,
	IN size_t       OutputBufferLength,
	IN size_t       InputBufferLength,
	IN ULONG        IoControlCode
)
{
	NTSTATUS            status = STATUS_SUCCESS;
	WDFDEVICE           device;
	PCROSFP_CONTEXT     devContext;
	BOOLEAN				completeRequest = TRUE;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	device = WdfIoQueueGetDevice(Queue);
	devContext = GetDeviceContext(device);

	PCROSFP_REQ_CONTEXT reqContext = GetRequestContext(Request);
	reqContext->devContext = devContext;

	CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
		"CrosFPEvtIoDeviceControl start\n");

	switch (IoControlCode)
	{
	case IOCTL_BIOMETRIC_GET_ATTRIBUTES:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested get fp attributes\n");
		status = GetFingerprintAttributes(Request);
		break;
	case IOCTL_BIOMETRIC_RESET:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested fp reset\n");
		status = STATUS_NOT_SUPPORTED;
		break;
	case IOCTL_BIOMETRIC_CALIBRATE:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested fp calibrate\n");
		status = CalibrateSensor(devContext, Request);
		break;
	case IOCTL_BIOMETRIC_GET_SENSOR_STATUS:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested get fp sensor status\n");
		status = GetSensorStatus(devContext, Request);
		break;
	case IOCTL_BIOMETRIC_CAPTURE_DATA:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested capture fp data\n");
		status = CaptureFpData(devContext, Request, &completeRequest);
		break;
	case IOCTL_BIOMETRIC_UPDATE_FIRMWARE:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested update fp firmware\n");
		status = STATUS_NOT_SUPPORTED;
		break;
	case IOCTL_BIOMETRIC_GET_SUPPORTED_ALGORITHMS:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested get fp algorithms\n");
		status = STATUS_NOT_SUPPORTED;
		break;
	case IOCTL_BIOMETRIC_VENDOR:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Requested raw ec command\n");
		status = CrosECIoctlXCmd(devContext, Request);
		break;
	default:
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Unknown FP Ioctl: 0x%x\n", IoControlCode);
		status = STATUS_NOT_SUPPORTED;
		break;
	}

	if (completeRequest) {
		WdfRequestComplete(Request, status);
	}

	return;
}