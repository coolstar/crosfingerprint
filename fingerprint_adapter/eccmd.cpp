#include "precomp.h"
#include "winbio_adapter.h"
#include <stdlib.h>

#include <winioctl.h>
#include <winbio_ioctl.h>
#include "../crosfingerprint/ec_commands.h"

typedef struct _CROSEC_COMMAND {
    UINT32 Version;
    UINT32 Command;
    UINT32 OutSize;
    UINT32 InSize;
    UINT32 Result;
    UINT8 Data[];
} CROSEC_COMMAND, * PCROSEC_COMMAND;

HRESULT ec_command(PWINBIO_PIPELINE Pipeline, int cmd, int version, const void* outdata, int outsize, void* indata, int insize) {
    if (!Pipeline) {
        return E_POINTER;
    }

    size_t size = sizeof(CROSEC_COMMAND) + max(outsize, insize);
    PCROSEC_COMMAND cmdStruct = (PCROSEC_COMMAND)malloc(size);
    if (!cmdStruct) {
        DebugLog("Failed to allocate cmd struct\n");
        return HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER);
    }
    RtlZeroMemory(cmdStruct, size);

    cmdStruct->Version = version;
    cmdStruct->Command = cmd;
    cmdStruct->OutSize = outsize;
    cmdStruct->InSize = insize;
    cmdStruct->Result = 0xff;

    RtlCopyMemory(cmdStruct->Data, outdata, outsize);

    size_t ret = 0;
    ULONG error;
    HRESULT hr = WbioSensorControlUnit(Pipeline,
        IOCTL_BIOMETRIC_VENDOR,
        (PUCHAR)cmdStruct,
        size,
        (PUCHAR)cmdStruct,
        size,
        &ret,
        &error);
    if (FAILED(hr)) {
        DebugLog("IOCTL failed\n");
        return HRESULT_FROM_WIN32(error);
    }

    RtlCopyMemory(indata, cmdStruct->Data, insize);

    if (cmdStruct->Result != 0) {
        return -cmdStruct->Result;
    }

    return S_OK;
}

HRESULT DownloadTemplate(PWINBIO_PIPELINE Pipeline, PUCHAR *outBuffer, UINT32 templateSize, int idx) {
    if (!Pipeline ||
        !outBuffer) {
        return E_POINTER;
    }

    *outBuffer = NULL;

    struct ec_response_get_protocol_info info;
    HRESULT hr = ec_command(Pipeline, EC_CMD_GET_PROTOCOL_INFO, 0, NULL, 0, &info, sizeof(info));
    if (FAILED(hr)) {
        DebugLog("Failed to get Protocol Info\n");
        return hr;
    }

    UINT32 ec_max_outsize = info.max_request_packet_size - sizeof(struct ec_host_request);
    UINT32 ec_max_insize = info.max_response_packet_size - sizeof(struct ec_host_response);
    const int max_attempts = 3;
    int num_attempts;

    PUCHAR buffer = (PUCHAR)malloc(templateSize);
    if (!buffer) {
        DebugLog("Failed to allocate buffer!\n");
        return WINBIO_E_DATABASE_WRITE_ERROR;
    }

    DebugLog("Downloading fingerprint template %d: %d bytes\n", idx, templateSize);

    UINT32 stride, size;
    size = templateSize;
    struct ec_params_fp_frame p;
    PUCHAR ptr = buffer;
    p.offset = (idx + 1) << FP_FRAME_INDEX_SHIFT;

    while (size) {
        stride = min(ec_max_insize, size);
        DebugLog("Downloading (%d bytes in stride)... ", stride);
        p.size = stride;
        num_attempts = 0;
        while (num_attempts < max_attempts) {
            num_attempts++;
            hr = ec_command(Pipeline, EC_CMD_FP_FRAME, 0, &p, sizeof(p), ptr, stride);
            if (SUCCEEDED(hr)) {
                DebugLog("success\n");
                break;
            }
            else if (hr == -EC_RES_ACCESS_DENIED) {
                DebugLog("access denied\n");
                break;
            }
            Sleep(100);
        }
        if (FAILED(hr)) {
            DebugLog("failed\n");
            free(buffer);
            return hr;
        }
        p.offset += stride;
        size -= stride;
        ptr += stride;
    }

    DebugLog("Download success!\n");
    *outBuffer = buffer;
    return hr;
}