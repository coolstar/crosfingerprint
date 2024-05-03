#include "crosfingerprint.h"
#include "ec_commands.h"
#include "crosfpdata.h"

static ULONG CrosFPDebugLevel = 0;
static ULONG CrosFPDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS ResetSensor(
	IN PCROSFP_CONTEXT   devContext,
	IN WDFREQUEST   Request
) {
	NTSTATUS status;
	PWINBIO_BLANK_PAYLOAD sensorStatus;
	SIZE_T outputBufferSize;

	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(WINBIO_BLANK_PAYLOAD), &sensorStatus, &outputBufferSize);
	if (!NT_SUCCESS(status)) {
		PDWORD payloadSize;
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DWORD), &payloadSize, &outputBufferSize);

		CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"WdfRequestRetrieveOutputBuffer sending DWORD\n");

		if (!NT_SUCCESS(status)) {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestRetrieveOutputBuffer failed %x\n", status);
			return status;
		}

		*payloadSize = sizeof(WINBIO_BLANK_PAYLOAD);
		WdfRequestSetInformation(Request, sizeof(DWORD));
		return status;
	}

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"WdfRequestRetrieveOutputBuffer sending WINBIO_BLANK_PAYLOAD\n");

	RtlZeroMemory(sensorStatus, outputBufferSize);
	sensorStatus->PayloadSize = sizeof(WINBIO_BLANK_PAYLOAD);
	sensorStatus->WinBioHresult = S_OK;

	WdfRequestSetInformation(Request, sizeof(WINBIO_BLANK_PAYLOAD));
	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"WdfRequestRetrieveOutputBuffer sent WINBIO_BLANK_PAYLOAD\n");
	return STATUS_SUCCESS;
}

NTSTATUS GetFingerprintAttributes(
	IN WDFREQUEST   Request
) {
	NTSTATUS status;
	PWINBIO_SENSOR_ATTRIBUTES sensorAttributes;
	SIZE_T outputBufferSize;

	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(WINBIO_SENSOR_ATTRIBUTES), &sensorAttributes, &outputBufferSize);
	if (!NT_SUCCESS(status)) {
		PDWORD payloadSize;
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DWORD), &payloadSize, &outputBufferSize);

		CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"WdfRequestRetrieveOutputBuffer sending DWORD\n");

		if (!NT_SUCCESS(status)) {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestRetrieveOutputBuffer failed %x\n", status);
			return status;
		}

		*payloadSize = sizeof(WINBIO_SENSOR_ATTRIBUTES);
		WdfRequestSetInformation(Request, sizeof(DWORD));
		return status;
	}

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"WdfRequestRetrieveOutputBuffer sending WINBIO_SENSOR_ATTRIBUTES\n");

	WCHAR MANUFACTURER_NAME[] = L"CoolStar";
	WCHAR MODEL_NAME[] = L"Chromebook Fingerprint MCU";
	WCHAR SERIAL_NUMBER[] = L"crosec-spi";
	
	RtlZeroMemory(sensorAttributes, outputBufferSize);
	sensorAttributes->PayloadSize = (DWORD)sizeof(WINBIO_SENSOR_ATTRIBUTES);
	sensorAttributes->WinBioHresult = S_OK;
	sensorAttributes->WinBioVersion.MajorVersion = WINBIO_WBDI_MAJOR_VERSION;
	sensorAttributes->WinBioVersion.MinorVersion = WINBIO_WBDI_MINOR_VERSION;
	sensorAttributes->SensorType = WINBIO_TYPE_FINGERPRINT;
	sensorAttributes->SensorSubType = WINBIO_FP_SENSOR_SUBTYPE_TOUCH;
	sensorAttributes->Capabilities = WINBIO_CAPABILITY_SENSOR | WINBIO_CAPABILITY_MATCHING | WINBIO_CAPABILITY_DATABASE | WINBIO_CAPABILITY_PROCESSING | WINBIO_CAPABILITY_ENCRYPTION;
	sensorAttributes->SupportedFormatEntries = 1;
	sensorAttributes->SupportedFormat[0].Owner = WINBIO_ANSI_381_FORMAT_OWNER;
	sensorAttributes->SupportedFormat[0].Type = WINBIO_ANSI_381_FORMAT_TYPE;
	RtlCopyMemory(sensorAttributes->ManufacturerName, MANUFACTURER_NAME, (wcslen(MANUFACTURER_NAME) + 1) * sizeof(WCHAR));
	RtlCopyMemory(sensorAttributes->ModelName, MODEL_NAME, (wcslen(MODEL_NAME) + 1) * sizeof(WCHAR));
	RtlCopyMemory(sensorAttributes->SerialNumber, SERIAL_NUMBER, (wcslen(SERIAL_NUMBER) + 1) * sizeof(WCHAR));
	sensorAttributes->FirmwareVersion.MajorVersion = 1;
	sensorAttributes->FirmwareVersion.MinorVersion = 0;

	WdfRequestSetInformation(Request, sizeof(WINBIO_SENSOR_ATTRIBUTES));

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"WdfRequestRetrieveOutputBuffer sent WINBIO_SENSOR_ATTRIBUTES\n");

	return status;
}

NTSTATUS GetSensorStatus(
	IN PCROSFP_CONTEXT   devContext,
	IN WDFREQUEST   Request
) {
	NTSTATUS status;
	PWINBIO_DIAGNOSTICS sensorStatus;
	SIZE_T outputBufferSize;

	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(WINBIO_DIAGNOSTICS), &sensorStatus, &outputBufferSize);
	if (!NT_SUCCESS(status)) {
		PDWORD payloadSize;
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DWORD), &payloadSize, &outputBufferSize);

		CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"WdfRequestRetrieveOutputBuffer sending DWORD\n");

		if (!NT_SUCCESS(status)) {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestRetrieveOutputBuffer failed %x\n", status);
			return status;
		}

		*payloadSize = sizeof(WINBIO_DIAGNOSTICS);
		WdfRequestSetInformation(Request, sizeof(DWORD));
		return status;
	}

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"WdfRequestRetrieveOutputBuffer sending WINBIO_DIAGNOSTICS\n");

	WINBIO_SENSOR_MODE sensorMode;
	status = CrosFPSensorStatus(devContext, &sensorMode);

	RtlZeroMemory(sensorStatus, outputBufferSize);
	sensorStatus->PayloadSize = sizeof(WINBIO_DIAGNOSTICS);
	sensorStatus->WinBioHresult = NT_SUCCESS(status) ? 0 : WINBIO_E_INVALID_SENSOR_MODE;
	sensorStatus->SensorStatus = sensorMode;

	WdfRequestSetInformation(Request, sizeof(WINBIO_DIAGNOSTICS));
	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"WdfRequestRetrieveOutputBuffer sent WINBIO_DIAGNOSTICS\n");
	return STATUS_SUCCESS;
}

NTSTATUS CalibrateSensor(
	IN PCROSFP_CONTEXT   devContext,
	IN WDFREQUEST   Request
) {
	NTSTATUS status;
	PWINBIO_CALIBRATION_INFO sensorCalibration;
	SIZE_T outputBufferSize;

	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(WINBIO_CALIBRATION_INFO), &sensorCalibration, &outputBufferSize);
	if (!NT_SUCCESS(status)) {
		PDWORD payloadSize;
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DWORD), &payloadSize, &outputBufferSize);

		CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"WdfRequestRetrieveOutputBuffer sending DWORD\n");

		if (!NT_SUCCESS(status)) {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestRetrieveOutputBuffer failed %x\n", status);
			return status;
		}

		*payloadSize = sizeof(WINBIO_CALIBRATION_INFO);
		WdfRequestSetInformation(Request, sizeof(DWORD));
		return status;
	}

	int i = 0;
	while (!devContext->DeviceReady && !devContext->DeviceCalibrated) {
		Sleep(200);
		i++;
	}
	CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
		"Ready after %d iters\n", i);
	if (!devContext->DeviceCalibrated){
		status = STATUS_DEVICE_CONFIGURATION_ERROR; //Failed to auto-calibrate
	}

	sensorCalibration->PayloadSize = sizeof(WINBIO_CALIBRATION_INFO);
	sensorCalibration->WinBioHresult = NT_SUCCESS(status) ? 0 : WINBIO_E_INVALID_DEVICE_STATE;

	WdfRequestSetInformation(Request, sizeof(WINBIO_CALIBRATION_INFO));
	return STATUS_SUCCESS;
}

void CancelFPRequest(
	WDFREQUEST Request
);

NTSTATUS
CaptureFpData(
	IN PCROSFP_CONTEXT   devContext,
	IN WDFREQUEST   Request,
	IN PBOOLEAN		CompleteRequest
) {
	NTSTATUS status;
	PWINBIO_CAPTURE_PARAMETERS CaptureParams;
	PCRFP_CAPTURE_DATA CaptureData;
	size_t bufferSize;
	status = WdfRequestRetrieveInputBuffer(Request, sizeof(WINBIO_CAPTURE_PARAMETERS), &CaptureParams, &bufferSize);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"Failed to get capture params\n");
		return status;
	}

	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(CRFP_CAPTURE_DATA), &CaptureData, &bufferSize);
	if (!NT_SUCCESS(status)) {
		PDWORD payloadSize;
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DWORD), &payloadSize, &bufferSize);

		CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"WdfRequestRetrieveOutputBuffer sending DWORD\n");

		if (!NT_SUCCESS(status)) {
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestRetrieveOutputBuffer failed %x\n", status);
			return status;
		}

		*payloadSize = sizeof(CRFP_CAPTURE_DATA);
		WdfRequestSetInformation(Request, sizeof(DWORD));
		return status;
	}

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"Capture Params Purpose 0x%x, Format (0x%x 0x%x), Flags 0x%x\n", CaptureParams->Purpose, CaptureParams->Format.Owner, CaptureParams->Format.Type, CaptureParams->Flags);

	CaptureData->PayloadSize = sizeof(CRFP_CAPTURE_DATA);
	CaptureData->SensorStatus = WINBIO_SENSOR_READY;
	CaptureData->RejectDetail = 0;
	CaptureData->DataSize = 0;
	CaptureData->FPMKBPValue = 0;

	if (CaptureParams->Format.Owner != WINBIO_ANSI_381_FORMAT_OWNER || CaptureParams->Format.Type != WINBIO_ANSI_381_FORMAT_TYPE) {
		CaptureData->WinBioHresult = WINBIO_E_UNSUPPORTED_DATA_FORMAT;
		WdfRequestSetInformation(Request, sizeof(CRFP_CAPTURE_DATA));
		return status;
	}

	if (CaptureParams->Flags != WINBIO_DATA_FLAG_RAW && CaptureParams->Flags != WINBIO_DATA_FLAG_PROCESSED) {
		CaptureData->WinBioHresult = WINBIO_E_UNSUPPORTED_DATA_TYPE;
		WdfRequestSetInformation(Request, sizeof(CRFP_CAPTURE_DATA));
		return status;
	}

	if (devContext->CurrentCapture) {
		WINBIO_SENSOR_MODE sensorMode;
		status = CrosFPSensorStatus(devContext, &sensorMode);

		CaptureData->WinBioHresult = WINBIO_E_DATA_COLLECTION_IN_PROGRESS;
	}
	else {
		struct ec_params_fp_mode p;
		struct ec_response_fp_mode r;

		p.mode = FP_MODE_DONT_CHANGE;

		if (CaptureParams->Purpose == WINBIO_PURPOSE_IDENTIFY || CaptureParams->Purpose == WINBIO_PURPOSE_VERIFY)
			p.mode = FP_MODE_CAPTURE | FP_MODE_MATCH | FP_MODE_FINGER_DOWN | FP_MODE_FINGER_UP;
		else if (CaptureParams->Purpose == WINBIO_PURPOSE_ENROLL ||
			CaptureParams->Purpose == WINBIO_PURPOSE_ENROLL_FOR_IDENTIFICATION ||
			CaptureParams->Purpose == WINBIO_PURPOSE_ENROLL_FOR_VERIFICATION)
			p.mode = FP_MODE_ENROLL_IMAGE | FP_MODE_ENROLL_SESSION | FP_MODE_FINGER_DOWN | FP_MODE_FINGER_UP;
		else if (CaptureParams->Purpose != 0) {
			CaptureData->WinBioHresult = WINBIO_E_UNSUPPORTED_PURPOSE;

			WdfRequestSetInformation(Request, sizeof(CRFP_CAPTURE_DATA));
			return status;
		}

		if (devContext->FingerUp) {

			status = cros_ec_command(devContext, EC_CMD_FP_MODE, 0, &p, sizeof(p), &r, sizeof(r));
			if (!NT_SUCCESS(status)) {
				return status;
			}
			devContext->NextMode = FP_MODE_DEEPSLEEP;

			CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
				"New Mode set to 0x%x\n", r.mode);
		}
		else {
			devContext->NextMode = p.mode;

			p.mode = (p.mode & FP_MODE_ENROLL_SESSION) | FP_MODE_FINGER_DOWN | FP_MODE_FINGER_UP;
			status = cros_ec_command(devContext, EC_CMD_FP_MODE, 0, &p, sizeof(p), &r, sizeof(r));
			if (!NT_SUCCESS(status)) {
				return status;
			}

			CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
				"Setting mode to 0x%x on finger up\n", devContext->NextMode);
		}
		*CompleteRequest = FALSE;
		devContext->CurrentCapture = Request;
		WdfRequestMarkCancelable(Request, CancelFPRequest);
	}

	WdfRequestSetInformation(Request, sizeof(CRFP_CAPTURE_DATA));

	return status;
}

void CancelFPRequest(
	WDFREQUEST Request
) {
	PCROSFP_REQ_CONTEXT reqContext = GetRequestContext(Request);
	CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
		"Cancel FP request!\n");

	PCROSFP_CONTEXT   devContext = reqContext->devContext;
	if (!devContext) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"No devContext attached to request!\n");
		return;
	}
	if (devContext->CurrentCapture == Request) {
		devContext->CurrentCapture = NULL;
		WdfRequestUnmarkCancelable(Request);
		WdfRequestComplete(Request, STATUS_CANCELLED);

		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"Marking as cancelled!\n");

		struct ec_params_fp_mode p;
		struct ec_response_fp_mode r;

		p.mode = FP_MODE_DEEPSLEEP;

		NTSTATUS status = cros_ec_command(devContext, EC_CMD_FP_MODE, 0, &p, sizeof(p), &r, sizeof(r));
		UNREFERENCED_PARAMETER(status);
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"Set deepsleep mode! 0x%x\n", status);
	}
}

void CompleteFPRequest(
	IN PCROSFP_CONTEXT   devContext,
	IN UINT32 fp_events
) {
	if ((fp_events & EC_MKBP_FP_FINGER_DOWN) == EC_MKBP_FP_FINGER_DOWN) {
		devContext->FingerUp = FALSE;

		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"Got finger down!\n");

	}
	
	if ((fp_events & EC_MKBP_FP_FINGER_UP) == EC_MKBP_FP_FINGER_UP) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"Finger Up!\n");
		devContext->FingerUp = TRUE;

		if (devContext->NextMode != FP_MODE_DEEPSLEEP) {
			struct ec_params_fp_mode p;
			p.mode = devContext->NextMode & ~FP_MODE_FINGER_UP;

			struct ec_response_fp_mode r;
			NTSTATUS status = cros_ec_command(devContext, EC_CMD_FP_MODE, 0, &p, sizeof(p), &r, sizeof(r));
			UNREFERENCED_PARAMETER(status);
			CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"Set mode to 0x%x (finger up): %x\n", p.mode, status);
		}
	}

	if (!devContext->CurrentCapture) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"No pending capture request!\n");
		return;
	}

	WDFREQUEST Request = devContext->CurrentCapture;

	PWINBIO_CAPTURE_PARAMETERS CaptureParams;
	PCRFP_CAPTURE_DATA CaptureData;
	size_t bufferSize;
	NTSTATUS status;

	status = WdfRequestRetrieveInputBuffer(Request, sizeof(WINBIO_CAPTURE_PARAMETERS), &CaptureParams, &bufferSize);
	if (!NT_SUCCESS(status)) {
		goto exit;
	}

	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(CRFP_CAPTURE_DATA), &CaptureData, &bufferSize);
	if (!NT_SUCCESS(status)) {
		goto exit;
	}

	if (fp_events & EC_MKBP_FP_ENROLL) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"Enroll error %d, progress %d!\n", EC_MKBP_FP_ERRCODE(fp_events), EC_MKBP_FP_ENROLL_PROGRESS(fp_events));
		devContext->NextMode = FP_MODE_DEEPSLEEP;
	}
	else if (fp_events & EC_MKBP_FP_MATCH) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"Match error %d, idx %d!\n", EC_MKBP_FP_ERRCODE(fp_events), EC_MKBP_FP_MATCH_IDX(fp_events));
		devContext->NextMode = FP_MODE_DEEPSLEEP;
	}
	else {
		return;
	}

	CaptureData->PayloadSize = sizeof(CRFP_CAPTURE_DATA);
	CaptureData->WinBioHresult = S_OK;
	CaptureData->SensorStatus = WINBIO_SENSOR_ACCEPT;
	CaptureData->RejectDetail = 0;
	CaptureData->DataSize = sizeof(DWORD);
	CaptureData->FPMKBPValue = fp_events;

	if (CaptureData->FPMKBPValue & EC_MKBP_FP_ENROLL) {
		switch (EC_MKBP_FP_ERRCODE(CaptureData->FPMKBPValue)) {
		case EC_MKBP_FP_ERR_ENROLL_LOW_QUALITY:
		case EC_MKBP_FP_ERR_ENROLL_IMMOBILE:
			CaptureData->SensorStatus = WINBIO_SENSOR_REJECT;
			CaptureData->RejectDetail = WINBIO_FP_POOR_QUALITY;
			break;
		case EC_MKBP_FP_ERR_ENROLL_LOW_COVERAGE:
			CaptureData->SensorStatus = WINBIO_SENSOR_REJECT;
			CaptureData->RejectDetail = WINBIO_FP_TOO_SHORT;
			break;
		default:
			break;
		}
	}
	else if (CaptureData->FPMKBPValue & EC_MKBP_FP_MATCH) {
		switch (EC_MKBP_FP_ERRCODE(CaptureData->FPMKBPValue)) {
		case EC_MKBP_FP_ERR_MATCH_NO_LOW_QUALITY:
			CaptureData->SensorStatus = WINBIO_SENSOR_REJECT;
			CaptureData->RejectDetail = WINBIO_FP_POOR_QUALITY;
			break;
		case EC_MKBP_FP_ERR_MATCH_NO_LOW_COVERAGE:
			CaptureData->SensorStatus = WINBIO_SENSOR_REJECT;
			CaptureData->RejectDetail = WINBIO_FP_TOO_SHORT;
			break;
		default:
			break;
		}
	}

exit:
	devContext->CurrentCapture = NULL;
	WdfRequestUnmarkCancelable(Request);
	WdfRequestComplete(Request, status);
}