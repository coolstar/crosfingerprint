#include "precomp.h"
#include "winbio_adapter.h"
#include <stdlib.h>

#include <winioctl.h>
#include <winbio_ioctl.h>

void DebugLog(const char* format, ...);

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

    DebugLog("Result: %d\n", cmdStruct->Result);
    if (cmdStruct->Result != 0) {
        return -cmdStruct->Result;
    }

    return S_OK;
}