#if !defined(_CROSFP_H_)
#define _CROSFP_H_

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#include <Windows.h>

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>

#include <winbio_types.h>
#include <winbio_err.h>
#include <winbio_ioctl.h>

#include <stdint.h>
#include <stdlib.h>

#include "spb.h"

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
    UINT8 Version;
    UINT16 Command;
    UINT16 OutSize;
    UINT16 InSize;
    UINT32 Result;
    UINT8 Data[];
} CROSEC_COMMAND, * PCROSEC_COMMAND;

typedef struct _CROSFP_CONTEXT
{

	WDFDEVICE FxDevice;

    WDFQUEUE Queue;

	SPB_CONTEXT SpbContext;

	WDFINTERRUPT Interrupt;
    WDFREQUEST CurrentCapture;

    BOOLEAN DeviceReady;
    BOOLEAN DeviceCalibrated;

} CROSFP_CONTEXT, *PCROSFP_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CROSFP_CONTEXT, GetDeviceContext)

//
// Function definitions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD CrosFPDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD CrosFPEvtDeviceAdd;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS CrosFPEvtWdmPreprocessMnQueryId;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL CrosFPEvtIoDeviceControl;

NTSTATUS CrosFPSensorStatus
(
    _In_ PCROSFP_CONTEXT devContext,
    PWINBIO_SENSOR_STATUS sensorMode
);

NTSTATUS CrosFPCheck(_In_ PCROSFP_CONTEXT devContext);

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

#if 1
void DebugLog(const char* format, ...);
#define CrosFPPrint(dbglevel, dbgcatagory, fmt, ...) {          \
    if (CrosFPDebugLevel <= dbglevel &&                         \
        (CrosFPDebugCatagories && dbgcatagory))                 \
	    {                                                           \
        DebugLog(fmt, __VA_ARGS__);                             \
	    }                                                           \
}
#else
#define CrosFPPrint(dbglevel, fmt, ...) {                       \
}
#endif

#endif