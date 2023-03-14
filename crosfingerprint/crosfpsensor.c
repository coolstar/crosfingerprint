#include "crosfingerprint.h"
#include "ec_commands.h"

static ULONG CrosFPDebugLevel = 0;
static ULONG CrosFPDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS CrosFPCheck(_In_ PCROSFP_CONTEXT devContext) {
	struct ec_params_hello p;
	struct ec_response_hello r;
	NTSTATUS status;

	p.in_data = 0xa0b0c0d0;

	status = cros_ec_command(devContext, EC_CMD_HELLO, 0, &p, sizeof(p), &r, sizeof(r));
	if (!NT_SUCCESS(status)) {
		return status;
	}

	if (r.out_data != 0xa1b2c3d4) {
		return STATUS_INVALID_DEVICE_STATE;
	}

	return STATUS_SUCCESS;
}

NTSTATUS CrosFPSensorStatus
(
	_In_ PCROSFP_CONTEXT devContext,
	PWINBIO_SENSOR_STATUS sensorMode
)
{
	struct ec_params_fp_mode p;
	struct ec_response_fp_mode r;
	NTSTATUS status;

	p.mode = FP_MODE_DONT_CHANGE;

	status = cros_ec_command(devContext, EC_CMD_FP_MODE, 0, &p, sizeof(p), &r, sizeof(r));
	if (!NT_SUCCESS(status)) {
		return status;
	}

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"FP Mode: 0x%x, Calibrated? %d\n", r.mode, devContext->DeviceCalibrated);

	if (!devContext->DeviceCalibrated) {
		*sensorMode = WINBIO_SENSOR_NOT_CALIBRATED;
	}
	else {
		*sensorMode = WINBIO_SENSOR_READY;
		if (r.mode & FP_MODE_SENSOR_MAINTENANCE)
			*sensorMode = WINBIO_SENSOR_BUSY;
		else if (r.mode & FP_MODE_ANY_CAPTURE)
			*sensorMode = WINBIO_SENSOR_READY;
	}
	return status;
}