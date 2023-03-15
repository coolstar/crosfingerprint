/*++
Copyright (c) Microsoft Corporation. All Rights Reserved.
Sample code. Dealpoint ID #843729.

Module Name:

spb.c

Abstract:

Contains all I2C-specific functionality

Environment:

Kernel mode

Revision History:

--*/

#include "crosfingerprint.h"
#include "uart.h"
#include <reshub.h>

static ULONG CrosFPDebugLevel = 100;
static ULONG CrosFPDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

VOID
UartTargetDeinitialize(
	IN WDFDEVICE FxDevice,
	IN UART_CONTEXT* UartContext
)
/*++

Routine Description:

This helper routine is used to free any members added to the SPB_CONTEXT,
note the SPB I/O target is parented to the device and will be
closed and free'd when the device is removed.

Arguments:

FxDevice   - Handle to the framework device object
SpbContext - Pointer to the current device context

Return Value:

NTSTATUS Status indicating success or failure

--*/
{
	UNREFERENCED_PARAMETER(FxDevice);

	//
	// Free any SPB_CONTEXT allocations here
	//
	if (UartContext->UartLock != NULL)
	{
		WdfObjectDelete(UartContext->UartLock);
	}
	if (UartContext->UartRequest != NULL)
	{
		WdfObjectDelete(UartContext->UartRequest);
	}
}

NTSTATUS
UartTargetInitialize(
	IN WDFDEVICE FxDevice,
	IN UART_CONTEXT* SpbContext
)
/*++

Routine Description:

This helper routine opens the UART I/O target and
initializes a request object used for the lifetime
of communication between this driver and YART.

Arguments:

FxDevice   - Handle to the framework device object
SpbContext - Pointer to the current device context

Return Value:

NTSTATUS Status indicating success or failure

--*/
{
	WDF_OBJECT_ATTRIBUTES objectAttributes;
	WDF_IO_TARGET_OPEN_PARAMS openParams;
	UNICODE_STRING uartDeviceName;
	WCHAR uartDeviceNameBuffer[RESOURCE_HUB_PATH_SIZE];
	NTSTATUS status;

	WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
	objectAttributes.ParentObject = FxDevice;

	status = WdfIoTargetCreate(
		FxDevice,
		&objectAttributes,
		&SpbContext->SpbIoTarget);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error creating IoTarget object - 0x%x\n",
			status);

		WdfObjectDelete(SpbContext->SpbIoTarget);
		goto exit;
	}

	RtlInitEmptyUnicodeString(
		&uartDeviceName,
		uartDeviceNameBuffer,
		sizeof(uartDeviceNameBuffer));

	status = RESOURCE_HUB_CREATE_PATH_FROM_ID(
		&uartDeviceName,
		SpbContext->UartResHubId.LowPart,
		SpbContext->UartResHubId.HighPart);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error creating UART resource hub path string - 0x%x\n",
			status);
		goto exit;
	}

	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
		&openParams,
		&uartDeviceName,
		(GENERIC_READ | GENERIC_WRITE));

	openParams.ShareAccess = 0;
	openParams.CreateDisposition = FILE_OPEN;
	openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;

	status = WdfIoTargetOpen(SpbContext->SpbIoTarget, &openParams);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error opening UART target for communication - 0x%x\n",
			status);
		goto exit;
	}

	//
	// Allocate a waitlock to guard access to the default buffers
	//
	status = WdfWaitLockCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		&SpbContext->UartLock);

	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error creating UART Waitlock - 0x%x\n",
			status);
		goto exit;
	}

	status = WdfRequestCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		SpbContext->SpbIoTarget,
		&SpbContext->UartRequest
	);
	if (!NT_SUCCESS(status))
	{
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error creating UART Request - 0x%x\n",
			status);
		goto exit;
	}

exit:

	if (!NT_SUCCESS(status))
	{
		UartTargetDeinitialize(FxDevice, SpbContext);
	}

	return status;
}

NTSTATUS
UartWrite(
	IN UART_CONTEXT* SpbContext,
	PVOID Data,
	ULONG Len
) {
	WdfWaitLockAcquire(SpbContext->UartLock, NULL);

	WDF_MEMORY_DESCRIPTOR memoryDescriptor;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, Data, Len);

	// Configure the write request to time out after 500 ms.

	WDF_REQUEST_SEND_OPTIONS requestOptions;
	WDF_REQUEST_SEND_OPTIONS_INIT(&requestOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
	requestOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(500);

	ULONG_PTR bytesWritten;
	NTSTATUS status = WdfIoTargetSendWriteSynchronously(
		SpbContext->SpbIoTarget,
		SpbContext->UartRequest,
		&memoryDescriptor,
		NULL,
		&requestOptions,
		&bytesWritten
	);

	if (bytesWritten != Len) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Bytes written (%lld) doesn't match length %ld\n",
			bytesWritten,
			Len);
	}

	WDF_REQUEST_REUSE_PARAMS  params;
	WDF_REQUEST_REUSE_PARAMS_INIT(
		&params,
		WDF_REQUEST_REUSE_NO_FLAGS,
		STATUS_SUCCESS
	);
	WdfRequestReuse(
		SpbContext->UartRequest,
		&params);

	WdfWaitLockRelease(SpbContext->UartLock, NULL);
	return status;
}

NTSTATUS
UartRead(
	IN UART_CONTEXT* SpbContext,
	PVOID Data,
	ULONG Len
) {
	WdfWaitLockAcquire(SpbContext->UartLock, NULL);

	WDF_MEMORY_DESCRIPTOR memoryDescriptor;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, Data, Len);

	// Configure the write request to time out after 500 ms.

	WDF_REQUEST_SEND_OPTIONS requestOptions;
	WDF_REQUEST_SEND_OPTIONS_INIT(&requestOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
	requestOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(500);

	ULONG_PTR bytesRead;
	NTSTATUS status = WdfIoTargetSendReadSynchronously(
		SpbContext->SpbIoTarget,
		SpbContext->UartRequest,
		&memoryDescriptor,
		NULL,
		&requestOptions,
		&bytesRead
	);

	if (bytesRead != Len) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Bytes read (%lld) doesn't match length %ld\n",
			bytesRead,
			Len);
	}

	WDF_REQUEST_REUSE_PARAMS  params;
	WDF_REQUEST_REUSE_PARAMS_INIT(
		&params,
		WDF_REQUEST_REUSE_NO_FLAGS,
		STATUS_SUCCESS
	);
	WdfRequestReuse(
		SpbContext->UartRequest,
		&params);

	WdfWaitLockRelease(SpbContext->UartLock, NULL);
	return status;
}