#include "crosfingerprint.h"
#include "ec_commands.h"

static ULONG CrosFPDebugLevel = 100;
static ULONG CrosFPDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS cros_ec_pkt_xfer_uart(
	PCROSFP_CONTEXT pDevice,
	PCROSEC_COMMAND msg
) {
	NTSTATUS status;

	unsigned int dout_len = sizeof(struct ec_host_request) + msg->OutSize;
	unsigned int din_len = sizeof(struct ec_host_response) + msg->InSize;

	UINT8* dout = malloc(dout_len);
	UINT8* din = malloc(din_len);
	if (!dout || !din) {
		status = STATUS_NO_MEMORY;
		goto out;
	}

	RtlZeroMemory(dout, dout_len);
	RtlZeroMemory(din, din_len);

	{ //prepare packet
		struct ec_host_request *request = (struct ec_host_request *)dout;
		request->struct_version = EC_HOST_REQUEST_VERSION;
		request->checksum = 0;
		request->command = (UINT16)msg->Command;
		request->command_version = (UINT8)msg->Version;
		request->reserved = 0;
		request->data_len = (UINT16)msg->OutSize;

		UINT8 csum = 0;
		for (int i = 0; i < sizeof(*request); i++) {
			csum += dout[i];
		}

		/* Copy data and update checksum */
		memcpy(dout + sizeof(*request), msg->Data, msg->OutSize);
		for (UINT32 i = 0; i < msg->OutSize; i++)
			csum += msg->Data[i];

		request->checksum = -csum;
	}

	status = UartWrite(&pDevice->UartContext, dout, dout_len);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error writing to UART: %x\n", status);
		goto out;
	}

	status = UartRead(&pDevice->UartContext, din, din_len);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error reading from UART: %x\n", status);
		goto out;
	}

	struct ec_host_response *response = (struct ec_host_response *)din;
	msg->Result = response->result;

	if (response->data_len > msg->InSize) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Resp too long (%d bytes, expected %d)\n", response->data_len, msg->InSize);
		status = STATUS_BUFFER_OVERFLOW;
		goto out;
	}

	/* Copy response packet to ec_msg data buffer */
	memcpy(msg->Data,
		din + sizeof(*response),
		response->data_len);

	UINT8 sum = 0;
	/* Add all response header bytes for checksum calculation */
	for (int i = 0; i < sizeof(*response); i++)
		sum += din[i];

	/* Copy response packet payload and compute checksum */
	for (int i = 0; i < response->data_len; i++)
		sum += msg->Data[i];

	if (sum) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Bad packet checksum calculated %x\n",
			sum);
		status = STATUS_DATA_CHECKSUM_ERROR;
	}

	status = STATUS_SUCCESS;

out:
	if (dout) {
		free(dout);
	}

	if (din) {
		free(din);
	}

	return status;
}