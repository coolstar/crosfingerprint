/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    SensorAdapter.cpp

Abstract:

    This module contains a stub implementation of a Sensor Adapter
    plug-in for the Windows Biometric service.

Author:

    -

Environment:

    Win32, user mode only.

Revision History:

NOTES:

    (None)

--*/

///////////////////////////////////////////////////////////////////////////////
//
// Header files...
//
///////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "winbio_adapter.h"
#include "SensorAdapter.h"
#include "../crosfingerprint/ec_commands.h"

#include <winioctl.h>
#include <winbio_types.h>
#include <winbio_err.h>
#include <winbio_ioctl.h>

///////////////////////////////////////////////////////////////////////////////
//
// Forward declarations for the Engine Adapter's interface routines...
//
///////////////////////////////////////////////////////////////////////////////
static HRESULT
WINAPI
SensorAdapterAttach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
SensorAdapterDetach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
SensorAdapterClearContext(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
SensorAdapterQueryStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_SENSOR_STATUS Status
    );

static HRESULT
WINAPI
SensorAdapterReset(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
SensorAdapterSetMode(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_SENSOR_MODE Mode
    );

static HRESULT
WINAPI
SensorAdapterSetIndicatorStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_INDICATOR_STATUS IndicatorStatus
    );

static HRESULT
WINAPI
SensorAdapterGetIndicatorStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_INDICATOR_STATUS IndicatorStatus
    );

static HRESULT
WINAPI
SensorAdapterStartCapture(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIR_PURPOSE Purpose,
    _Out_ LPOVERLAPPED *Overlapped
    );

static HRESULT
WINAPI
SensorAdapterFinishCapture(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    );

static HRESULT
WINAPI
SensorAdapterClearCaptureBuffer(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
SensorAdapterExportSensorData(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_BIR *SampleBuffer,
    _Out_ PSIZE_T SampleSize
    );

static HRESULT
WINAPI
SensorAdapterCancel(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
SensorAdapterPushDataToEngine(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIR_PURPOSE Purpose,
    _In_ WINBIO_BIR_DATA_FLAGS Flags,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    );

static HRESULT
WINAPI
SensorAdapterControlUnit(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ ULONG ControlCode,
    _In_ PUCHAR SendBuffer,
    _In_ SIZE_T SendBufferSize,
    _In_ PUCHAR ReceiveBuffer,
    _In_ SIZE_T ReceiveBufferSize,
    _Out_ PSIZE_T ReceiveDataSize,
    _Out_ PULONG OperationStatus
    );

static HRESULT
WINAPI
SensorAdapterControlUnitPrivileged(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ ULONG ControlCode,
    _In_ PUCHAR SendBuffer,
    _In_ SIZE_T SendBufferSize,
    _In_ PUCHAR ReceiveBuffer,
    _In_ SIZE_T ReceiveBufferSize,
    _Out_ PSIZE_T ReceiveDataSize,
    _Out_ PULONG OperationStatus
    );
//-----------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////////////////
//
// Interface dispatch table
//
///////////////////////////////////////////////////////////////////////////////
static WINBIO_SENSOR_INTERFACE g_SensorInterface = {
    WINBIO_STORAGE_INTERFACE_VERSION_1,
    WINBIO_ADAPTER_TYPE_SENSOR,
    sizeof(WINBIO_SENSOR_INTERFACE),
    {0xa545298c, 0xec34, 0x4306, {0x84, 0x12, 0x83, 0x12, 0x5d, 0xca, 0xfa, 0xe1}},

    SensorAdapterAttach,
    SensorAdapterDetach,
    SensorAdapterClearContext,
    SensorAdapterQueryStatus,
    SensorAdapterReset,
    SensorAdapterSetMode,
    SensorAdapterSetIndicatorStatus,
    SensorAdapterGetIndicatorStatus,
    SensorAdapterStartCapture,
    SensorAdapterFinishCapture,
    SensorAdapterExportSensorData,
    SensorAdapterCancel,
    SensorAdapterPushDataToEngine,
    SensorAdapterControlUnit,
    SensorAdapterControlUnitPrivileged
};
//-----------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////////////////
//
// Well-known interface-discovery function exported by the Sensor Adapter
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
WbioQuerySensorInterface(
    _Out_ PWINBIO_SENSOR_INTERFACE *SensorInterface
    )
{
    *SensorInterface = &g_SensorInterface;
    return S_OK;
}
//-----------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////////////////
//
// Storage Adapter action routines
//
///////////////////////////////////////////////////////////////////////////////
static HRESULT
WINAPI
SensorAdapterAttach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called SensorAdapterAttach\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    PWINIBIO_SENSOR_CONTEXT newContext = (PWINIBIO_SENSOR_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINIBIO_SENSOR_CONTEXT));
    if (!newContext) {
        return E_OUTOFMEMORY;
    }

    Pipeline->SensorContext = newContext;
    RtlZeroMemory(&Pipeline->SensorContext->Overlapped, sizeof(OVERLAPPED));
    Pipeline->SensorContext->Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!Pipeline->SensorContext->Overlapped.hEvent) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DebugLog("Handles: sensor 0x%lx, engine 0x%lx, storage 0x%lx\n", Pipeline->SensorHandle, Pipeline->EngineHandle, Pipeline->StorageHandle);

cleanup:
    return hr;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterDetach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called SensorAdapterDetach\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->SensorContext) {
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, Pipeline->SensorContext);
        Pipeline->SensorContext = NULL;
    }

cleanup:
    return hr;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterClearContext(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{

    DebugLog("Called SensorAdapterClearContext\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    RtlZeroMemory(&Pipeline->SensorContext->CaptureData, sizeof(Pipeline->SensorContext->CaptureData));

cleanup:
    return hr;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterQueryStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_SENSOR_STATUS Status
    )
{
    WINBIO_DIAGNOSTICS Diag;
    DWORD BytesReturned;

    DebugLog("Called SensorAdapterQueryStatus\n");

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        return E_POINTER;
    }

    while (TRUE) {
        if (!DeviceIoControl(Pipeline->SensorHandle,
            IOCTL_BIOMETRIC_GET_SENSOR_STATUS,
            NULL,
            0,
            &Diag,
            sizeof(WINBIO_DIAGNOSTICS),
            &BytesReturned,
            NULL)) {
            DWORD LastError = GetLastError();
            if (LastError == ERROR_CANCELLED || LastError == ERROR_OPERATION_ABORTED) {
                return WINBIO_E_CANCELED;
            }
            else {
                return HRESULT_FROM_WIN32(LastError);
            }
        }
        if (BytesReturned != 4) {
            *Status = Diag.SensorStatus;
            return S_OK;
        }
    }
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterReset(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    UNREFERENCED_PARAMETER(Pipeline);

    DebugLog("Called SensorAdapterReset\n");

    return E_NOTIMPL;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterSetMode(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_SENSOR_MODE Mode
    )
{
    DebugLog("Called SensorAdapterSetMode (mode 0x%x)\n", Mode);

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        DebugLog("SensorAdapterSetMode No Pointer!\n");
        return E_POINTER;
    }

    if (Mode != WINBIO_SENSOR_ADVANCED_MODE && Mode != WINBIO_SENSOR_SLEEP_MODE) {
        DebugLog("SensorAdapterSetMode Not Impl!\n");
        return E_NOTIMPL;
    }

    DebugLog("SensorAdapterSetMode Success!\n");

    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterSetIndicatorStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_INDICATOR_STATUS IndicatorStatus
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(IndicatorStatus);

    DebugLog("Called SensorAdapterSetIndicatorStatus (status 0x%x)\n", IndicatorStatus);

    return E_NOTIMPL;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterGetIndicatorStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_INDICATOR_STATUS IndicatorStatus
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(IndicatorStatus);

    DebugLog("Called SensorAdapterGetIndicatorStatus\n");

    return E_NOTIMPL;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterStartCapture(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIR_PURPOSE Purpose,
    _Out_ LPOVERLAPPED* Overlapped
)
{
    DebugLog("Called SensorAdapterStartCapture, purpose 0x%x\n", Purpose);

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        return E_POINTER;
    }

    *Overlapped = &Pipeline->SensorContext->Overlapped;

    DWORD BytesReturned;

    WINBIO_DIAGNOSTICS Diag = { 0 };
    if (!DeviceIoControl(Pipeline->SensorHandle,
        IOCTL_BIOMETRIC_GET_SENSOR_STATUS,
        NULL,
        0,
        &Diag,
        sizeof(WINBIO_DIAGNOSTICS),
        &BytesReturned,
        NULL)) {
        DWORD LastError = GetLastError();
        DebugLog("IOCTL_BIOMETRIC_GET_SENSOR_STATUS failed 0x%x 0x%x\n", LastError, HRESULT_FROM_WIN32(LastError));
        return HRESULT_FROM_WIN32(LastError);
    }
    if (FAILED(Diag.WinBioHresult)) {
        DebugLog("IOCTL_BIOMETRIC_GET_SENSOR_STATUS errored 0x%x\n", Diag.WinBioHresult);
        return Diag.WinBioHresult;
    }

    if ((Diag.SensorStatus & WINBIO_SENSOR_NOT_CALIBRATED) == WINBIO_SENSOR_NOT_CALIBRATED) {
        WINBIO_CALIBRATION_INFO Calib = { 0 };

        if (!DeviceIoControl(Pipeline->SensorHandle,
            IOCTL_BIOMETRIC_CALIBRATE,
            NULL,
            0,
            &Calib,
            sizeof(WINBIO_CALIBRATION_INFO),
            &BytesReturned,
            NULL)) {
            DWORD LastError = GetLastError();
            DebugLog("IOCTL_BIOMETRIC_CALIBRATE failed 0x%x 0x%x\n", LastError, HRESULT_FROM_WIN32(LastError));
            return HRESULT_FROM_WIN32(LastError);
        }
        if (FAILED(Calib.WinBioHresult)) {
            DebugLog("IOCTL_BIOMETRIC_CALIBRATE errored 0x%x\n", Calib.WinBioHresult);
            return Calib.WinBioHresult;
        }
    }

    WINBIO_CAPTURE_PARAMETERS Parameters = { 0 };
    Parameters.PayloadSize = sizeof(WINBIO_CAPTURE_PARAMETERS);
    Parameters.Purpose = Purpose;
    Parameters.Flags = WINBIO_DATA_FLAG_PROCESSED;

    RtlZeroMemory(&Pipeline->SensorContext->CaptureData, sizeof(CRFP_CAPTURE_DATA));

    if (!DeviceIoControl(Pipeline->SensorHandle,
        IOCTL_BIOMETRIC_CAPTURE_DATA,
        &Parameters,
        sizeof(WINBIO_CAPTURE_PARAMETERS),
        &Pipeline->SensorContext->CaptureData,
        sizeof(CRFP_CAPTURE_DATA),
        &BytesReturned,
        &Pipeline->SensorContext->Overlapped)) {

        DWORD LastError = GetLastError();
        if (GetLastError() != ERROR_IO_PENDING) {
            DebugLog("IOCTL_BIOMETRIC_CAPTURE_DATA failed 0x%x 0x%x\n", LastError, HRESULT_FROM_WIN32(LastError));
            return HRESULT_FROM_WIN32(LastError);
        }
    }
    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterFinishCapture(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    )
{
    UNREFERENCED_PARAMETER(RejectDetail);

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        return E_POINTER;
    }

    SetLastError(0);

    DWORD BytesReturned;
    if (!GetOverlappedResult(Pipeline->SensorHandle, &Pipeline->SensorContext->Overlapped, &BytesReturned, TRUE)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    UINT32 MKBPResult = Pipeline->SensorContext->CaptureData.FPMKBPValue;
    *RejectDetail = 0;
    
    if (MKBPResult & EC_MKBP_FP_ENROLL) {
        switch (EC_MKBP_FP_ERRCODE(MKBPResult)) {
        case EC_MKBP_FP_ERR_ENROLL_LOW_QUALITY:
            *RejectDetail = WINBIO_FP_POOR_QUALITY;
            break;
        case EC_MKBP_FP_ERR_ENROLL_LOW_COVERAGE:
            *RejectDetail = WINBIO_FP_TOO_SHORT;
            break;
        default:
            break;
        }
    }
    else if (MKBPResult & EC_MKBP_FP_MATCH) {
        switch (EC_MKBP_FP_ERRCODE(MKBPResult)) {
        case EC_MKBP_FP_ERR_MATCH_NO_LOW_QUALITY:
            *RejectDetail = WINBIO_FP_POOR_QUALITY;
            break;
        case EC_MKBP_FP_ERR_MATCH_NO_LOW_COVERAGE:
            *RejectDetail = WINBIO_FP_TOO_SHORT;
            break;
        default:
            break;
        }
    }

    DebugLog("Called SensorAdapterFinishCapture MKBP: %x, ret %x, rej %x\n", Pipeline->SensorContext->CaptureData.FPMKBPValue, Pipeline->SensorContext->CaptureData.WinBioHresult, *RejectDetail);

    return Pipeline->SensorContext->CaptureData.WinBioHresult;
}
///////////////////////////////////////////////////////////////////////////////

//
// Export raw capture buffer
//
static HRESULT
WINAPI
SensorAdapterExportSensorData(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_BIR *SampleBuffer,
    _Out_ PSIZE_T SampleSize
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(SampleBuffer);
    UNREFERENCED_PARAMETER(SampleSize);

    DebugLog("Called SensorAdapterExportSensorData\n");

    return E_NOTIMPL;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterCancel(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called SensorAdapterCancel\n");

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        return E_POINTER;
    }

    if (!CancelIoEx(Pipeline->SensorHandle, NULL)) {
        DWORD LastError = GetLastError();
        if (LastError != ERROR_NOT_FOUND) {
            DebugLog("SensorAdapterCancel failed with %d\n", LastError);
            return HRESULT_FROM_WIN32(LastError);
        }
    }

    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////

//
// Push current sample into the Engine and
// convert it into a feature set for use in
// additional processing.
//
static HRESULT
WINAPI
SensorAdapterPushDataToEngine(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIR_PURPOSE Purpose,
    _In_ WINBIO_BIR_DATA_FLAGS Flags,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    )
{
    UNREFERENCED_PARAMETER(Purpose);
    UNREFERENCED_PARAMETER(Flags);

    DebugLog("Called SensorAdapterPushDataToEngine. Purpose 0x%x, Flags 0x%x\n", Purpose, Flags);

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(RejectDetail))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_SENSOR_CONTEXT sensorContext =
        (PWINIBIO_SENSOR_CONTEXT)Pipeline->SensorContext;

    // Verify the state of the pipeline.
    if (sensorContext == NULL)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    if (sensorContext->CaptureData.PayloadSize >= sizeof(CRFP_CAPTURE_DATA) &&
        (sensorContext->CaptureData.FPMKBPValue & (EC_MKBP_FP_ENROLL | EC_MKBP_FP_MATCH)) != 0)
    {
        // There is valid capture data in the Pipeline. Call the 
        // WbioEngineAcceptSampleData function to notify the engine adapter, but
        // retain ownership of the buffer in the sensor adapter. 
        // WbioEngineAcceptSampleData is a wrapper function declared in the
        // Winbio_adapter.h header file.
        DebugLog("Forwarding to engine. Purpose 0x%x, Flags 0x%x. Raw MKBP value 0x%x\n", Purpose, Flags, sensorContext->CaptureData.FPMKBPValue);

        hr = WbioEngineAcceptSampleData(
            Pipeline,
            (PWINBIO_BIR)&sensorContext->CaptureData.FPMKBPValue,
            sizeof(UINT32),
            Purpose,
            RejectDetail
        );
    }
    else if (sensorContext->CaptureData.PayloadSize >= sizeof(CRFP_CAPTURE_DATA) &&
        sensorContext->CaptureData.WinBioHresult == WINBIO_E_BAD_CAPTURE)
    {
        // The most recent capture was not acceptable.  Do not attempt to push 
        // the sample to the engine. Instead, simply return the reject detail
        // information generated by the previous capture.
        hr = sensorContext->CaptureData.WinBioHresult;
        *RejectDetail = WINBIO_FP_POOR_QUALITY;
        DebugLog("Bad capture\n");
    }
    else
    {
        DebugLog("No capture data (Payload Size %d)\n", sensorContext->CaptureData.PayloadSize);
        hr = WINBIO_E_NO_CAPTURE_DATA;
    }

cleanup:
    return hr;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterControlUnit(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ ULONG ControlCode,
    _In_ PUCHAR SendBuffer,
    _In_ SIZE_T SendBufferSize,
    _In_ PUCHAR ReceiveBuffer,
    _In_ SIZE_T ReceiveBufferSize,
    _Out_ PSIZE_T ReceiveDataSize,
    _Out_ PULONG OperationStatus
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(ControlCode);
    UNREFERENCED_PARAMETER(SendBuffer);
    UNREFERENCED_PARAMETER(SendBufferSize);
    UNREFERENCED_PARAMETER(ReceiveBuffer);
    UNREFERENCED_PARAMETER(ReceiveBufferSize);
    UNREFERENCED_PARAMETER(ReceiveDataSize);
    UNREFERENCED_PARAMETER(OperationStatus);

    DebugLog("Called SensorAdapterControlUnit\n");

    return E_NOTIMPL;
}
///////////////////////////////////////////////////////////////////////////////

static HRESULT
WINAPI
SensorAdapterControlUnitPrivileged(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ ULONG ControlCode,
    _In_ PUCHAR SendBuffer,
    _In_ SIZE_T SendBufferSize,
    _In_ PUCHAR ReceiveBuffer,
    _In_ SIZE_T ReceiveBufferSize,
    _Out_ PSIZE_T ReceiveDataSize,
    _Out_ PULONG OperationStatus
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(ControlCode);
    UNREFERENCED_PARAMETER(SendBuffer);
    UNREFERENCED_PARAMETER(SendBufferSize);
    UNREFERENCED_PARAMETER(ReceiveBuffer);
    UNREFERENCED_PARAMETER(ReceiveBufferSize);
    UNREFERENCED_PARAMETER(ReceiveDataSize);
    UNREFERENCED_PARAMETER(OperationStatus);

    DebugLog("Called SensorAdapterControlUnitPrivileged\n");

    return E_NOTIMPL;
}
///////////////////////////////////////////////////////////////////////////////