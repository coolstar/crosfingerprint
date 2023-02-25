#if !defined(_CROSFP_H_)
#define _CROSFP_H_

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#include <wdm.h>

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>

#include <acpiioct.h>
#include <ntstrsafe.h>

#include <stdint.h>

#include "spb.h"

//
// String definitions
//

#define DRIVERNAME                 "crosfingerprint.sys: "

#define CROSFP_POOL_TAG            (ULONG) 'pFrC'

#define true 1
#define false 0

typedef struct _CROSEC_COMMAND {
    UINT32 Version;
    UINT32 Command;
    UINT32 OutSize;
    UINT32 InSize;
    UINT32 Result;
    UINT8 Data[];
} CROSEC_COMMAND, * PCROSEC_COMMAND;

typedef struct _CROSFP_CONTEXT
{

	WDFDEVICE FxDevice;

	WDFQUEUE ReportQueue;

	SPB_CONTEXT SpbContext;

	WDFINTERRUPT Interrupt;

} CROSFP_CONTEXT, *PCROSFP_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CROSFP_CONTEXT, GetDeviceContext)

//
// Function definitions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD CrosFPDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD CrosFPEvtDeviceAdd;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS CrosFPEvtWdmPreprocessMnQueryId;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL CrosFPEvtInternalDeviceControl;

NTSTATUS cros_ec_pkt_xfer(
    PCROSFP_CONTEXT pDevice,
    PCROSEC_COMMAND msg
);

NTSTATUS cros_ec_command(
    PCROSFP_CONTEXT pDevice,
    int command, int version,
    const void* outdata, int outsize,
    void* indata, int insize
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

#if 1
#define CrosFPPrint(dbglevel, dbgcatagory, fmt, ...) {          \
    if (CrosFPDebugLevel >= dbglevel &&                         \
        (CrosFPDebugCatagories && dbgcatagory))                 \
	    {                                                           \
        DbgPrint(DRIVERNAME);                                   \
        DbgPrint(fmt, __VA_ARGS__);                             \
	    }                                                           \
}
#else
#define CrosFPPrint(dbglevel, fmt, ...) {                       \
}
#endif

#endif