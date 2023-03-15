#include "crosfingerprint.h"
#include "ec_commands.h"

NTSTATUS cros_ec_pkt_xfer_spi(
	PCROSFP_CONTEXT pDevice,
	PCROSEC_COMMAND msg
);
NTSTATUS cros_ec_pkt_xfer_uart(
	PCROSFP_CONTEXT pDevice,
	PCROSEC_COMMAND msg
);

NTSTATUS cros_ec_pkt_xfer(
	PCROSFP_CONTEXT pDevice,
	PCROSEC_COMMAND msg
)
{
	switch (pDevice->IoType)
	{
	case CROSFP_TYPESPI:
		return cros_ec_pkt_xfer_spi(pDevice, msg);
	case CROSFP_TYPEUART:
		return cros_ec_pkt_xfer_uart(pDevice, msg);
	default:
		return STATUS_INVALID_DEVICE_STATE;
	}
}

NTSTATUS cros_ec_command(
	PCROSFP_CONTEXT pDevice,
	UINT16 command, UINT8 version,
	const void* outdata, UINT16 outsize,
	void* indata, UINT16 insize
) {
	PCROSEC_COMMAND msg = malloc(sizeof(CROSEC_COMMAND) + max(insize, outsize));
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

	free(msg);

	return status;
}

/**
 * Get the versions of the command supported by the EC.
 *
 * @param cmd		Command
 * @param pmask		Destination for version mask; will be set to 0 on
 *			error.
 */
static NTSTATUS cros_ec_get_cmd_versions(PCROSFP_CONTEXT pDevice, UINT16 cmd, UINT32* pmask) {
	struct ec_params_get_cmd_versions_v1 pver_v1;
	struct ec_params_get_cmd_versions pver;
	struct ec_response_get_cmd_versions rver;
	NTSTATUS status;

	*pmask = 0;

	pver_v1.cmd = cmd;
	status = cros_ec_command(pDevice, EC_CMD_GET_CMD_VERSIONS, 1, &pver_v1, sizeof(pver_v1),
		&rver, sizeof(rver));

	if (!NT_SUCCESS(status)) {
		pver.cmd = (UINT8)cmd;
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
BOOLEAN cros_ec_cmd_version_supported(PCROSFP_CONTEXT pDevice, UINT16 cmd, UINT8 ver)
{
	uint32_t mask = 0;

	if (NT_SUCCESS(cros_ec_get_cmd_versions(pDevice, cmd, &mask)))
		return false;

	return (mask & EC_VER_MASK(ver)) ? true : false;
}