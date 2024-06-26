#if !defined(_CROSFP_H_)
#define _CROSFP_H_

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#ifndef _KERNEL_MODE
// This is a user-mode driver
#include <windows.h>
#else
// This is a kernel-mode driver
#include <wdm.h>
#include <ntstrsafe.h>
#define PDWORD DWORD*
#endif

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>

#ifndef _KERNEL_MODE
#include <winbio_types.h>
#include <winbio_err.h>
#include <winbio_ioctl.h>

#include <stdint.h>
#include <stdlib.h>
#else
#define S_OK 0
#include "winbio_km.h"
#define HRESULT_FROM_NT(x)      ((HRESULT) ((x) | 0x10000000))

#define malloc(size) ExAllocatePoolZero(NonPagedPool, size, CROSFP_POOL_TAG);
#define free ExFreePool

#define Sleep(x) do { \
LARGE_INTEGER Interval; \
Interval.QuadPart = -x * 10 * 1000; \
KeDelayExecutionThread(KernelMode, FALSE, &Interval); \
} while (0);
#endif

#include "spb.h"
#include "uart.h"

//
// String definitions
//

#define DRIVERNAME                 "crosfingerprint.sys: "
#define CROSFP_POOL_TAG            (ULONG) 'pFrC'

#define true 1
#define false 0

#define FP_MODE_ANY_CAPTURE \
	(FP_MODE_CAPTURE | FP_MODE_ENROLL_IMAGE | FP_MODE_MATCH)
#define FP_MODE_ANY_DETECT_FINGER \
	(FP_MODE_FINGER_DOWN | FP_MODE_FINGER_UP | FP_MODE_ANY_CAPTURE)
#define FP_MODE_ANY_WAIT_IRQ (FP_MODE_FINGER_DOWN | FP_MODE_ANY_CAPTURE)

typedef struct _CROSEC_COMMAND {
    UINT32 Version;
    UINT32 Command;
    UINT32 OutSize;
    UINT32 InSize;
    UINT32 Result;
#pragma warning(disable:4200)
    UINT8 Data[];
} CROSEC_COMMAND, * PCROSEC_COMMAND;

typedef enum CROSFP_TYPE {
    CROSFP_TYPESPI,
    CROSFP_TYPEUART
} CROSFP_TYPE, *PCROSFP_TYPE;

typedef struct _CROSFP_CONTEXT
{

	WDFDEVICE FxDevice;

    WDFQUEUE Queue;

    WDFWAITLOCK IoLock;
    CROSFP_TYPE IoType;
    union {
        SPB_CONTEXT SpbContext;
        UART_CONTEXT UartContext;
    } IoContext;
    UINT64 LastTransferTick;

	WDFINTERRUPT Interrupt;
    WDFREQUEST CurrentCapture;
    BOOLEAN    FingerUp;
    UINT32     NextMode;

    UINT16     MaxOutsize;
    UINT16     MaxInsize;

    UINT32     TemplateSize;
    UINT16     TemplateMax;
    UINT16     ValidTemplates;

    BOOLEAN DevicePowered;
    BOOLEAN DeviceReady;
    BOOLEAN DeviceCalibrated;

} CROSFP_CONTEXT, *PCROSFP_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CROSFP_CONTEXT, GetDeviceContext)

typedef struct _CROSFP_REQ_CONTEXT
{
    PCROSFP_CONTEXT devContext;
} CROSFP_REQ_CONTEXT, *PCROSFP_REQ_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CROSFP_REQ_CONTEXT, GetRequestContext)


//
// Function definitions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD CrosFPDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD CrosFPEvtDeviceAdd;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS CrosFPEvtWdmPreprocessMnQueryId;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL CrosFPEvtIoDeviceControl;

NTSTATUS CrosECIoctlXCmd(_In_ PCROSFP_CONTEXT pDevice, _In_ WDFREQUEST Request);

NTSTATUS CrosFPSensorStatus
(
    _In_ PCROSFP_CONTEXT devContext,
    PWINBIO_SENSOR_STATUS sensorMode
);

NTSTATUS ResetSensor(
    IN PCROSFP_CONTEXT   devContext,
    IN WDFREQUEST   Request
);

NTSTATUS GetFingerprintAttributes(
    IN WDFREQUEST   Request
);

NTSTATUS GetSensorStatus(
    IN PCROSFP_CONTEXT   devContext,
    IN WDFREQUEST   Request
);

NTSTATUS CalibrateSensor(
    IN PCROSFP_CONTEXT   devContext,
    IN WDFREQUEST   Request
);

NTSTATUS
CaptureFpData(
    IN PCROSFP_CONTEXT   devContext,
    IN WDFREQUEST   Request,
    IN PBOOLEAN		CompleteRequest
);

void CompleteFPRequest(
    IN PCROSFP_CONTEXT   devContext,
    IN UINT32 fp_events
);

NTSTATUS cros_ec_pkt_xfer(
    PCROSFP_CONTEXT pDevice,
    PCROSEC_COMMAND msg
);

NTSTATUS cros_ec_command(
    PCROSFP_CONTEXT pDevice,
    UINT16 command, UINT8 version,
    const void* outdata, UINT16 outsize,
    void* indata, UINT16 insize
);

//
// Helper macros
//

#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_INFO    2
#define DEBUG_LEVEL_VERBOSE 3

#define DBG_INIT  1
#define DBG_PNP   2
#define DBG_IOCTL 4

#if 0

#ifndef _KERNEL_MODE
#include <time.h>
#ifdef __cplusplus
extern "C"
#endif
void DebugLog_internal(const char* format, ...);

#define DebugLog(fmt, ...) \
do {\
time_t mytime = time(NULL); \
char time_str[26];\
ctime_s(time_str, sizeof(time_str), &mytime);\
time_str[strlen(time_str) - 1] = '\0'; \
DebugLog_internal("[%s] " fmt, time_str, __VA_ARGS__);\
} while(0)
#else
#define DebugLog DbgPrint
#endif

#define CrosFPPrint(dbglevel, dbgcatagory, fmt, ...) {          \
    if (CrosFPDebugLevel <= dbglevel &&                         \
        (CrosFPDebugCatagories && dbgcatagory))                 \
	    {                                                           \
        DebugLog(fmt, __VA_ARGS__);                             \
	    }                                                           \
}
#else
#define DebugLog(x, ...)
#define CrosFPPrint(dbglevel, fmt, ...) {                       \
}
#endif

#endif