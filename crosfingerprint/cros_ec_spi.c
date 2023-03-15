#include "crosfingerprint.h"
#include "ec_commands.h"

/* The header byte, which follows the preamble */
#define EC_MSG_HEADER			0xec

/*
 * Number of EC preamble bytes we read at a time. Since it takes
 * about 400-500us for the EC to respond there is not a lot of
 * point in tuning this. If the EC could respond faster then
 * we could increase this so that might expect the preamble and
 * message to occur in a single transaction. However, the maximum
 * SPI transfer size is 256 bytes, so at 5MHz we need a response
 * time of perhaps <320us (200 bytes / 1600 bits).
 */
#define EC_MSG_PREAMBLE_COUNT		32
 
 /*
 * Allow for a long time for the EC to respond.  We support i2c
 * tunneling and support fairly long messages for the tunnel (249
 * bytes long at the moment).  If we're talking to a 100 kHz device
 * on the other end and need to transfer ~256 bytes, then we need:
 *  10 us/bit * ~10 bits/byte * ~256 bytes = ~25ms
 *
 * We'll wait 8 times that to handle clock stretching and other
 * paranoia.  Note that some battery gas gauge ICs claim to have a
 * clock stretch of 144ms in rare situations.  That's incentive for
 * not directly passing i2c through, but it's too late for that for
 * existing hardware.
 *
 * It's pretty unlikely that we'll really see a 249 byte tunnel in
 * anything other than testing.  If this was more common we might
 * consider having slow commands like this require a GET_STATUS
 * wait loop.  The 'flash write' command would be another candidate
 * for this, clocking in at 2-3ms.
 */
#define EC_MSG_DEADLINE_MS		200

 /*
   * Time between raising the SPI chip select (for the end of a
   * transaction) and dropping it again (for the next transaction).
   * If we go too fast, the EC will miss the transaction. We know that we
   * need at least 70 us with the 16 MHz STM32 EC, so go with 200 us to be
   * safe.
   */
#define EC_SPI_RECOVERY_TIME_NS	(200 * 1000)


static ULONG CrosFPDebugLevel = 100;
static ULONG CrosFPDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS cros_ec_pkt_xfer_spi(
	PCROSFP_CONTEXT pDevice,
	PCROSEC_COMMAND msg
) {
	NTSTATUS status;

	BOOLEAN controllerLocked = FALSE;

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

	WdfWaitLockAcquire(pDevice->IoLock, NULL);

	status = SpbLockController(&pDevice->SpbContext);
	if (!NT_SUCCESS(status)) {
		goto out;
	}
	controllerLocked = TRUE;

	{ //prepare packet
		struct ec_host_request *request = (struct ec_host_request *)dout;
		request->struct_version = EC_HOST_REQUEST_VERSION;
		request->checksum = 0;
		request->command = msg->Command;
		request->command_version = msg->Version;
		request->reserved = 0;
		request->data_len = msg->OutSize;

		UINT8 csum = 0;
		for (int i = 0; i < sizeof(*request); i++) {
			csum += dout[i];
		}

		/* Copy data and update checksum */
		memcpy(dout + sizeof(*request), msg->Data, msg->OutSize);
		for (int i = 0; i < msg->OutSize; i++)
			csum += msg->Data[i];

		request->checksum = -csum;
	}

	status = SpbWriteDataSynchronously(&pDevice->SpbContext, NULL, 0);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error waking up SPI: %x\n", status);
		goto out;
	}

	status = SpbWriteDataSynchronously(&pDevice->SpbContext, dout, dout_len);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error sending SPI message: %x\n", status);
		goto out;
	}

	DWORD Timeout = GetTickCount() + EC_MSG_DEADLINE_MS;
	while (TRUE) {
		UINT8 byte = 0;
		status = SpbReadDataSynchronously(&pDevice->SpbContext, &byte, sizeof(byte));
		if (!NT_SUCCESS(status)) {
			break;
		}

		if (GetTickCount() > Timeout) {
			status = STATUS_IO_TIMEOUT;
			break;
		}
		
		if (byte == EC_SPI_FRAME_START)
			break;
	}

	if (!NT_SUCCESS(status)) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error receiving SPI framing byte: %x\n", status);
		goto out;
	}

	status = SpbReadDataSynchronously(&pDevice->SpbContext, din, din_len);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error receiving SPI message: %x\n", status);
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

	CrosFPPrint(
		DEBUG_LEVEL_ERROR,
		DBG_IOCTL,
		"Pkt xfer success\n"
	);

	status = STATUS_SUCCESS;

out:
	if (controllerLocked)
		SpbUnlockController(&pDevice->SpbContext);

	WdfWaitLockRelease(pDevice->IoLock);

	if (dout) {
		free(dout);
	}

	if (din) {
		free(din);
	}

	return status;
}