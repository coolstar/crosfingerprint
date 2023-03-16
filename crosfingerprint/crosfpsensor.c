#include "crosfingerprint.h"
#include "ec_commands.h"

static ULONG CrosFPDebugLevel = 0;
static ULONG CrosFPDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS CrosFPSensorStatus
(
	_In_ PCROSFP_CONTEXT devContext,
	PWINBIO_SENSOR_STATUS sensorMode
)
{
	NTSTATUS status = STATUS_SUCCESS;

	CrosFPPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"Calibrated? %d\n", devContext->DeviceCalibrated);

	if (!devContext->DeviceCalibrated) {
		*sensorMode = WINBIO_SENSOR_NOT_CALIBRATED;
	}
	else {
		*sensorMode = WINBIO_SENSOR_READY;
	}
	return status;
}

#define NT_RETURN_IF(status, condition)                \
	do {                                           \
		const NTSTATUS __statusRet = (status); \
		if((condition)) {                      \
			return __statusRet;            \
		}                                      \
	} while((void)0, 0)

NTSTATUS CrosECIoctlXCmd(_In_ PCROSFP_CONTEXT pDevice, _In_ WDFREQUEST Request) {
	int i = 0;
	while (!pDevice->DeviceReady) {
		Sleep(200);
		i++;
	}
	CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
		"Ready after %d iters\n", i);

	PCROSEC_COMMAND cmd;
	size_t cmdLen;
	NTSTATUS status = WdfRequestRetrieveInputBuffer(Request, sizeof(*cmd), (PVOID*)&cmd, &cmdLen);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL, "Failed to get input buffer\n");
		return status;
	}

	PCROSEC_COMMAND outCmd;
	size_t outLen;
	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*cmd), &outCmd, &outLen);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL, "Failed to get output buffer\n");
		return status;
	}

	// User tried to send/receive too much data
	NT_RETURN_IF(STATUS_BUFFER_OVERFLOW, cmdLen > (sizeof(CROSEC_COMMAND) + pDevice->MaxOutsize));
	NT_RETURN_IF(STATUS_BUFFER_OVERFLOW, outLen > (sizeof(CROSEC_COMMAND) + pDevice->MaxInsize));
	// User tried to send/receive more bytes than they offered in storage
	NT_RETURN_IF(STATUS_BUFFER_TOO_SMALL, cmdLen < (sizeof(CROSEC_COMMAND) + cmd->OutSize));
	NT_RETURN_IF(STATUS_BUFFER_TOO_SMALL, outLen < (sizeof(CROSEC_COMMAND) + cmd->InSize));


	PCROSEC_COMMAND xferCmd = malloc(max(cmdLen, outLen));
	if (!xferCmd) {
		return STATUS_NO_MEMORY;
	}

	RtlCopyMemory(xferCmd, cmd, sizeof(*cmd)); //Copy header
	RtlCopyMemory(xferCmd->Data, cmd->Data, cmd->OutSize);

	NTSTATUS cmdStatus = cros_ec_pkt_xfer(pDevice, xferCmd);

	RtlCopyMemory(outCmd, xferCmd, sizeof(*cmd)); //Copy header
	RtlCopyMemory(outCmd->Data, xferCmd->Data, cmd->InSize);

	free(xferCmd);

	if (!NT_SUCCESS(cmdStatus)) {
		outCmd->Result = cmdStatus;
	}

	int requiredReplySize = sizeof(CROSEC_COMMAND) + cmd->InSize;
	if (requiredReplySize > outLen) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	WdfRequestSetInformation(Request, requiredReplySize);
	return STATUS_SUCCESS;
}