#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#include <wdm.h>

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>

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

static NTSTATUS cros_ec_spi_receive_response(PCROSFP_CONTEXT pDevice, int need_len, int foundPreamble, int readLen, UINT8* buf, unsigned int buf_len) {
	if (buf_len < EC_MSG_PREAMBLE_COUNT)
		return STATUS_INVALID_PARAMETER;

	LARGE_INTEGER CurrentTimestamp;
	KeQuerySystemTime(&CurrentTimestamp);

	LARGE_INTEGER deadline = CurrentTimestamp;
	deadline.QuadPart += (EC_MSG_DEADLINE_MS * 1000 * 10);

	NTSTATUS status;
	UINT8* ptr, * end;

	if (foundPreamble > -1) {
		ptr = buf + foundPreamble;
		end = buf + readLen;
	}
	else {
		while (true) {
			status = SpbReadDataSynchronously(&pDevice->SpbContext, buf, EC_MSG_PREAMBLE_COUNT);
			if (!NT_SUCCESS(status)) {
				return status;
			}

			ptr = buf;
			for (end = ptr + EC_MSG_PREAMBLE_COUNT; ptr != end; ptr++) {
				if (*ptr == EC_SPI_FRAME_START) {
					CrosFPPrint(
						DEBUG_LEVEL_INFO,
						DBG_IOCTL,
						"msg found at %zd\n",
						ptr - buf);
					break;
				}
			}
			if (ptr != end)
				break;

			KeQuerySystemTime(&CurrentTimestamp);
			if (deadline.QuadPart > CurrentTimestamp.QuadPart) {
				return STATUS_IO_TIMEOUT;
			}
		}
	}

	/*
	 * ptr now points to the header byte. Copy any valid data to the
	 * start of our buffer
	 */
	int todo = end - ++ptr;
	todo = min(todo, need_len);
	RtlMoveMemory(buf, ptr, todo);
	ptr = buf + todo;
	CrosFPPrint(
		DEBUG_LEVEL_INFO,
		DBG_IOCTL,
		"need %d, got %d bytes from preamble\n",
		need_len, todo);

	/* Receive data until we have it all */
	while (need_len > 0) {
		/*
		 * We can't support transfers larger than the SPI FIFO size
		 * unless we have DMA.
		 */
		todo = min(need_len, 256);

		status = SpbReadDataSynchronously(&pDevice->SpbContext, ptr, todo);
		if (!NT_SUCCESS(status)) {
			return status;
		}

		ptr += todo;
		need_len -= todo;
	}
	return STATUS_SUCCESS;
}

NTSTATUS cros_ec_pkt_xfer(
	PCROSFP_CONTEXT pDevice,
	PCROSEC_COMMAND msg
) {
	NTSTATUS status;

	BOOLEAN controllerLocked = FALSE;

	unsigned int dout_len = sizeof(struct ec_host_request) + msg->OutSize;
	unsigned int din_len = sizeof(struct ec_host_response) + msg->InSize;
	din_len = max(din_len, dout_len);

	UINT8* dout = ExAllocatePoolWithTag(NonPagedPool, dout_len, CROSFP_POOL_TAG);
	UINT8* din = ExAllocatePoolWithTag(NonPagedPool, din_len, CROSFP_POOL_TAG);
	if (!dout || !din) {
		status = STATUS_NO_MEMORY;
		goto out;
	}

	RtlZeroMemory(dout, dout_len);
	RtlZeroMemory(din, din_len);

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

	status = SpbXferDataSynchronously(&pDevice->SpbContext, dout, dout_len, din, dout_len);
	if (!NT_SUCCESS(status)) {
		CrosFPPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error sending SPI message: %x\n", status);
		goto out;
	}

	INT foundPreamble = -1;
	for (int i = 0; i < dout_len; i++) {
		UINT8 rx_byte = din[i];
		if (rx_byte == EC_SPI_FRAME_START) {
			CrosFPPrint(
				DEBUG_LEVEL_INFO,
				DBG_IOCTL,
				"Found preamble at %d\n", i);
			foundPreamble = i;
			break;
		}
		if (rx_byte == EC_SPI_PAST_END ||
			rx_byte == EC_SPI_RX_BAD_DATA ||
			rx_byte == EC_SPI_NOT_READY) {
			CrosFPPrint(
				DEBUG_LEVEL_ERROR,
				DBG_IOCTL,
				"Read 0x%x. Retry needed\n", rx_byte);
			status = STATUS_RETRY;
			break;
		}
	}
	if (NT_SUCCESS(status)) {
		status = cros_ec_spi_receive_response(pDevice, sizeof(struct ec_host_response) + msg->InSize, foundPreamble, dout_len, din, din_len);
	}

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

	status = STATUS_SUCCESS;

out:
	if (controllerLocked)
		SpbUnlockController(&pDevice->SpbContext);

	if (dout) {
		ExFreePoolWithTag(dout, CROSFP_POOL_TAG);
	}

	if (din) {
		ExFreePoolWithTag(din, CROSFP_POOL_TAG);
	}

	return status;
}

NTSTATUS cros_ec_command (
	PCROSFP_CONTEXT pDevice,
	int command, int version,
	const void* outdata, int outsize,
	void* indata, int insize
) {
	PCROSEC_COMMAND msg = ExAllocatePoolWithTag(NonPagedPool, sizeof(CROSEC_COMMAND) + max(insize, outsize), CROSFP_POOL_TAG);
	if (!msg) {
		return STATUS_NO_MEMORY;
	}

	msg->Command = command;
	msg->Version = version;
	msg->OutSize = outsize;
	msg->InSize = insize;

	memcpy(msg->Data, outdata, outsize);
	
	NTSTATUS status = cros_ec_pkt_xfer(pDevice, msg);

	memcpy(indata, msg->Data, insize);

	ExFreePoolWithTag(msg, CROSFP_POOL_TAG);

	return status;
}

/**
 * Get the versions of the command supported by the EC.
 *
 * @param cmd		Command
 * @param pmask		Destination for version mask; will be set to 0 on
 *			error.
 */
static NTSTATUS cros_ec_get_cmd_versions(PCROSFP_CONTEXT pDevice, int cmd, UINT32* pmask) {
	struct ec_params_get_cmd_versions_v1 pver_v1;
	struct ec_params_get_cmd_versions pver;
	struct ec_response_get_cmd_versions rver;
	NTSTATUS status;

	*pmask = 0;

	pver_v1.cmd = cmd;
	status = cros_ec_command(pDevice, EC_CMD_GET_CMD_VERSIONS, 1, &pver_v1, sizeof(pver_v1),
		&rver, sizeof(rver));

	if (!NT_SUCCESS(status)) {
		pver.cmd = cmd;
		status = cros_ec_command(pDevice, EC_CMD_GET_CMD_VERSIONS, 0, &pver, sizeof(pver),
			&rver, sizeof(rver));
	}

	*pmask = rver.version_mask;
	return status;
}

/**
 * Return non-zero if the EC supports the command and version
 *
 * @param cmd		Command to check
 * @param ver		Version to check
 * @return non-zero if command version supported; 0 if not.
 */
BOOLEAN cros_ec_cmd_version_supported(PCROSFP_CONTEXT pDevice, int cmd, int ver)
{
	uint32_t mask = 0;

	if (NT_SUCCESS(cros_ec_get_cmd_versions(pDevice, cmd, &mask)))
		return false;

	return (mask & EC_VER_MASK(ver)) ? true : false;
}