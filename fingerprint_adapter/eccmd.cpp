#include "precomp.h"
#include "winbio_adapter.h"
#include <stdlib.h>

#include <winioctl.h>
#include <winbio_ioctl.h>

#pragma warning(disable:4200)  // suppress nameless struct/union warning
typedef struct _CROSEC_COMMAND {
    UINT32 Version;
    UINT32 Command;
    UINT32 OutSize;
    UINT32 InSize;
    UINT32 Result;
    UINT8 Data[];
} CROSEC_COMMAND, * PCROSEC_COMMAND;
#pragma warning(default:4200)

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
    RtlZeroMemory(indata, insize);

    size_t ret = 0;
    ULONG error;
    HRESULT hr = WbioSensorControlUnit(Pipeline,
        IOCTL_BIOMETRIC_VENDOR,
        (PUCHAR)cmdStruct,
        sizeof(CROSEC_COMMAND) + outsize,
        (PUCHAR)cmdStruct,
        sizeof(CROSEC_COMMAND) + insize,
        &ret,
        &error);
    if (FAILED(hr)) {
        DebugLog("IOCTL failed 0x%x\n", error);
        return HRESULT_FROM_WIN32(error);
    }

    if (cmdStruct->Result != 0) {
        DebugLog("Bad result from EC: %d\n", cmdStruct->Result);
        return -cmdStruct->Result;
    }

    RtlCopyMemory(indata, cmdStruct->Data, insize);

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
        return WINBIO_E_INVALID_DEVICE_STATE;
    }

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
        p.size = stride;
        num_attempts = 0;
        while (num_attempts < max_attempts) {
            num_attempts++;
            hr = ec_command(Pipeline, EC_CMD_FP_FRAME, 0, &p, sizeof(p), ptr, stride);
            if (SUCCEEDED(hr)) {
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
            return WINBIO_E_BAD_CAPTURE;
        }
        p.offset += stride;
        size -= stride;
        ptr += stride;
    }

    /*struct ec_fp_template_encryption_metadata* metadata = (struct ec_fp_template_encryption_metadata*)buffer;
    DebugLog("Template version: %d\n", metadata->struct_version);*/

    DebugLog("Download success!\n");
    *outBuffer = buffer;
    return hr;
}

HRESULT ResetFPContext(PWINBIO_PIPELINE Pipeline) {
    if (!Pipeline) {
        return E_POINTER;
    }

    DebugLog("Resetting FP Context\n");

    UINT32 userID[] = {1, 2, 3, 4, 5, 6, 7, 8};
    struct ec_params_fp_context_v1 p;

    p.action = FP_CONTEXT_ASYNC;
    RtlCopyMemory(p.userid, userID, sizeof(userID));

    int tries = 20;

    struct ec_params_fp_mode mode_p;
    struct ec_response_fp_mode mode_r;

    mode_p.mode = 0;

    HRESULT hr = ec_command(Pipeline, EC_CMD_FP_MODE, 0, &mode_p, sizeof(mode_p), &mode_r, sizeof(mode_r));
    if (FAILED(hr)) {
        DebugLog("Reset FP Context (set fp mode): %d\n", hr);
        hr = WINBIO_E_CONFIGURATION_FAILURE;
        goto cleanup;
    }

    DebugLog("Reset FP Context (read fp mode): 0x%x\n", mode_r.mode);

    if ((mode_r.mode & (FP_MODE_RESET_SENSOR | FP_MODE_SENSOR_MAINTENANCE)) != 0) {
        DebugLog("Waiting for sensor reset / maintenance\n");
        Sleep(100);
    }

    hr = ec_command(Pipeline, EC_CMD_FP_CONTEXT, 1, &p, sizeof(p), NULL, 0);
    if (FAILED(hr)) {
        DebugLog("Reset FP Context (async): %d\n", hr);
        hr = WINBIO_E_INVALID_SENSOR_MODE;
        goto cleanup;
    }

    while (tries--) {
        Sleep(100);

        p.action = FP_CONTEXT_GET_RESULT;
        hr = ec_command(Pipeline, EC_CMD_FP_CONTEXT, 1, &p, sizeof(p), NULL, 0);
        if (SUCCEEDED(hr)) {
            break;
        }

        if (hr != -EC_RES_BUSY) {
            DebugLog("Failed to reset FP Context: %d\n", hr);
            break;
        }
        hr = WINBIO_E_INVALID_DEVICE_STATE;
    }

cleanup:
    DebugLog("Reset FP Context: %d\n", hr);
    return hr;
}

HRESULT UploadTemplate(PWINBIO_PIPELINE Pipeline, PUCHAR buffer, UINT32 templateSize) {
    if (!Pipeline) {
        return E_POINTER;
    }

    struct ec_response_get_protocol_info info;
    HRESULT hr = ec_command(Pipeline, EC_CMD_GET_PROTOCOL_INFO, 0, NULL, 0, &info, sizeof(info));
    if (FAILED(hr)) {
        DebugLog("Failed to get Protocol Info\n");
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        return hr;
    }

    UINT32 ec_max_outsize = info.max_request_packet_size - sizeof(struct ec_host_request);

    struct ec_params_fp_template* p = (struct ec_params_fp_template* )malloc(ec_max_outsize);
    if (!p) {
        return WINBIO_E_DATABASE_WRITE_ERROR;
    }

    DebugLog("Uploading fingerprint template: %d bytes\n", templateSize);

    UINT32 max_chunk = ec_max_outsize - offsetof(struct ec_params_fp_template, data) - 4;
    UINT32 size, offset;
    size = templateSize;
    offset = 0;

    while (size) {
        UINT32 tlen = min(max_chunk, size);

        p->offset = offset;
        p->size = tlen;
        size -= tlen;
        if (!size)
            p->size |= FP_TEMPLATE_COMMIT;
        RtlCopyMemory(p->data, buffer + offset, tlen);

        for (int tries = 0; tries < 3; tries++) {
            hr = ec_command(Pipeline, EC_CMD_FP_TEMPLATE, 0, p,
                tlen + offsetof(struct ec_params_fp_template, data),
                NULL, 0);
            if (SUCCEEDED(hr)) {
                break;
            }
        }
        if (FAILED(hr)) {
            DebugLog("Upload failed: %d\n", hr);
            hr = WINBIO_E_BAD_CAPTURE;
            break;
        }
        offset += tlen;
    }
    
    free(p);
    return hr;
}

/*HRESULT PrintConsole(PWINBIO_PIPELINE Pipeline) {
    if (!Pipeline) {
        return E_POINTER;
    }

    struct ec_response_get_protocol_info info;
    HRESULT hr = ec_command(Pipeline, EC_CMD_GET_PROTOCOL_INFO, 0, NULL, 0, &info, sizeof(info));
    if (FAILED(hr)) {
        DebugLog("Failed to get Protocol Info\n");
        return hr;
    }

    UINT32 ec_max_outsize = info.max_request_packet_size - sizeof(struct ec_host_request);
    UINT32 ec_max_insize = info.max_response_packet_size - sizeof(struct ec_host_response);

    PUCHAR inbuf = (PUCHAR)malloc(ec_max_insize);
    hr = ec_command(Pipeline, EC_CMD_CONSOLE_SNAPSHOT, 0, NULL, 0, NULL, 0);
    if (FAILED(hr)) {
        DebugLog("Failed to get console snapshot\n");
        goto cleanup;
    }

    while (true) {
        hr = ec_command(Pipeline, EC_CMD_CONSOLE_READ, 0, NULL, 0, inbuf, ec_max_insize);
        if (FAILED(hr) || !*inbuf) {
            goto cleanup;
        }

        inbuf[ec_max_insize - 1] = '\0';
        DebugLog("%s", inbuf);
    }

cleanup:
    free(inbuf);
    return hr;
}*/