#include <Windows.h>
#include <stdio.h>
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

HRESULT ec_command(HANDLE device, int cmd, int version, const void* outdata, int outsize, void* indata, int insize) {
    if (device == INVALID_HANDLE_VALUE || !device) {
        DebugLog("Failed to open CrosEC FP device\n");
        return HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST);
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

    DWORD ret = 0;
    if (!DeviceIoControl(device, IOCTL_BIOMETRIC_VENDOR, cmdStruct, (DWORD)size, cmdStruct, (DWORD)size, &ret, NULL)) {
        DebugLog("IOCTL failed\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    RtlCopyMemory(indata, cmdStruct->Data, insize);

    DebugLog("Result: %d\n", cmdStruct->Result);
    if (cmdStruct->Result != 0) {
        return -cmdStruct->Result;
    }

    return S_OK;
}