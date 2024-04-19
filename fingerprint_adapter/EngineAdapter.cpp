/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    EngineAdapter.cpp

Abstract:

    This module contains a stub implementation of an Engine Adapter
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
#include "EngineAdapter.h"
#include "StorageAdapter.h"
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
//
// Forward declarations for the Engine Adapter's interface routines...
//
///////////////////////////////////////////////////////////////////////////////
static HRESULT
WINAPI
EngineAdapterAttach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
EngineAdapterDetach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
EngineAdapterClearContext(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
EngineAdapterEndOperation(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
EngineAdapterQueryPreferredFormat(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REGISTERED_FORMAT StandardFormat,
    _Out_ PWINBIO_UUID VendorFormat
    );

static HRESULT
WINAPI
EngineAdapterQueryIndexVectorSize(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T IndexElementCount
    );

static HRESULT
WINAPI
EngineAdapterQueryHashAlgorithms(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T AlgorithmCount,
    _Out_ PSIZE_T AlgorithmBufferSize,
    _Out_ PUCHAR *AlgorithmBuffer
    );

static HRESULT
WINAPI
EngineAdapterSetHashAlgorithm(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ SIZE_T AlgorithmBufferSize,
    _In_ PUCHAR AlgorithmBuffer
    );

static HRESULT
WINAPI
EngineAdapterQuerySampleHint(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T SampleHint
    );

static HRESULT
WINAPI
EngineAdapterAcceptSampleData(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_BIR SampleBuffer,
    _In_ SIZE_T SampleSize,
    _In_ WINBIO_BIR_PURPOSE Purpose,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    );

static HRESULT
WINAPI
EngineAdapterExportEngineData(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIR_DATA_FLAGS Flags,
    _Out_ PWINBIO_BIR *SampleBuffer,
    _Out_ PSIZE_T SampleSize
    );

static HRESULT
WINAPI
EngineAdapterVerifyFeatureSet(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _Out_ PBOOLEAN Match,
    _Out_ PUCHAR *PayloadBlob,
    _Out_ PSIZE_T PayloadBlobSize,
    _Out_ PUCHAR *HashValue,
    _Out_ PSIZE_T HashSize,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    );

static HRESULT
WINAPI
EngineAdapterIdentifyFeatureSet(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_IDENTITY Identity,
    _Out_ PWINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _Out_ PUCHAR *PayloadBlob,
    _Out_ PSIZE_T PayloadBlobSize,
    _Out_ PUCHAR *HashValue,
    _Out_ PSIZE_T HashSize,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    );

static HRESULT
WINAPI
EngineAdapterCreateEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
EngineAdapterUpdateEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    );

static HRESULT
WINAPI
EngineAdapterGetEnrollmentStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    );

static HRESULT
WINAPI
EngineAdapterGetEnrollmentHash(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PUCHAR *HashValue,
    _Out_ PSIZE_T HashSize
    );

static HRESULT
WINAPI
EngineAdapterCheckForDuplicate(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_IDENTITY Identity,
    _Out_ PWINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _Out_ PBOOLEAN Duplicate
    );

static HRESULT
WINAPI
EngineAdapterCommitEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _In_ PUCHAR PayloadBlob,
    _In_ SIZE_T PayloadBlobSize
    );

static HRESULT
WINAPI
EngineAdapterDiscardEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
EngineAdapterControlUnit(
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
EngineAdapterControlUnitPrivileged(
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
static WINBIO_ENGINE_INTERFACE g_EngineInterface = {
    WINBIO_ENGINE_INTERFACE_VERSION_1,
    WINBIO_ADAPTER_TYPE_ENGINE,
    sizeof(WINBIO_ENGINE_INTERFACE),
    {0xb876fdc8, 0x34e7, 0x471a, {0x82, 0xc8, 0x9c, 0xba, 0x6a, 0x35, 0x38, 0xec}},

    EngineAdapterAttach,
    EngineAdapterDetach,
    EngineAdapterClearContext,
    EngineAdapterQueryPreferredFormat,
    EngineAdapterQueryIndexVectorSize,
    EngineAdapterQueryHashAlgorithms,
    EngineAdapterSetHashAlgorithm,
    EngineAdapterQuerySampleHint,
    EngineAdapterAcceptSampleData,
    EngineAdapterExportEngineData,
    EngineAdapterVerifyFeatureSet,
    EngineAdapterIdentifyFeatureSet,
    EngineAdapterCreateEnrollment,
    EngineAdapterUpdateEnrollment,
    EngineAdapterGetEnrollmentStatus,
    EngineAdapterGetEnrollmentHash,
    EngineAdapterCheckForDuplicate,
    EngineAdapterCommitEnrollment,
    EngineAdapterDiscardEnrollment,
    EngineAdapterControlUnit,
    EngineAdapterControlUnitPrivileged
};
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
// Well-known interface-discovery function exported by the Engine Adapter
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
WbioQueryEngineInterface(
    _Out_ PWINBIO_ENGINE_INTERFACE *EngineInterface
    )
{
    if (!ARGUMENT_PRESENT(EngineInterface)) {
        return E_POINTER;
    }
    *EngineInterface = &g_EngineInterface;
    return S_OK;
}
//-----------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////////////////
//
// Engine Adapter action routines
//
///////////////////////////////////////////////////////////////////////////////
static HRESULT
WINAPI
EngineAdapterAttach(
    _Inout_ PWINBIO_PIPELINE Pipeline
)
{
    DebugLog("Called EngineAdapterAttach\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    PWINIBIO_ENGINE_CONTEXT newContext = (PWINIBIO_ENGINE_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINIBIO_ENGINE_CONTEXT));
    if (!newContext) {
        return E_OUTOFMEMORY;
    }

    Pipeline->EngineContext = newContext;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterDetach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called EngineAdapterDetach\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->EngineContext) {
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, Pipeline->EngineContext);
        Pipeline->EngineContext = NULL;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterClearContext(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called EngineAdapterClearContext\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    if (context == NULL)
    {
        goto cleanup;
    }

    context->Enrollment.InProgress = FALSE;
    context->Enrollment.EnrollmentProgress = 0;
    context->LastMKBPValue = 0;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterQueryPreferredFormat(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REGISTERED_FORMAT StandardFormat,
    _Out_ PWINBIO_UUID VendorFormat
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(StandardFormat);
    UNREFERENCED_PARAMETER(VendorFormat);

    DebugLog("Called EngineAdapterQueryPreferredFormat\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterQueryIndexVectorSize(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T IndexElementCount
    )
{
    DebugLog("Called EngineAdapterQueryIndexVectorSize\n");

    HRESULT hr = S_OK;
    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(IndexElementCount))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;
    if (!context) {
        hr = E_POINTER;
        goto cleanup;
    }

    // Specify the number of index vector elements supported by your adapter. This can
    // be any positive value or zero. Return zero if your adapter does not support placing 
    // templates into buckets. That is, return zero if your adapter does not support index 
    // vectors.
    *IndexElementCount = 1;


cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterQueryHashAlgorithms(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T AlgorithmCount,
    _Out_ PSIZE_T AlgorithmBufferSize,
    _Out_ PUCHAR *AlgorithmBuffer
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(AlgorithmCount);
    UNREFERENCED_PARAMETER(AlgorithmBufferSize);
    UNREFERENCED_PARAMETER(AlgorithmBuffer);

    DebugLog("Called EngineAdapterQueryHashAlgorithms\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterSetHashAlgorithm(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ SIZE_T AlgorithmBufferSize,
    _In_ PUCHAR AlgorithmBuffer
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(AlgorithmBufferSize);
    UNREFERENCED_PARAMETER(AlgorithmBuffer);

    DebugLog("Called EngineAdapterSetHashAlgorithm\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterQuerySampleHint(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T SampleHint
    )
{
    HRESULT hr = S_OK;
    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(SampleHint))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    DebugLog("Called EngineAdapterQuerySampleHint\n");

    PWINIBIO_STORAGE_CONTEXT storageContext = Pipeline->StorageContext;
    if (!storageContext) {
        hr = E_POINTER;
        goto cleanup;
    }

    if (storageContext->FingerWidth > (storageContext->FingerHeight * 3))
        *SampleHint = 10;
    else if (storageContext->FingerHeight > (storageContext->FingerWidth * 3))
        *SampleHint = 10;
    else
        *SampleHint = 5;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterAcceptSampleData(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_BIR SampleBuffer,
    _In_ SIZE_T SampleSize,
    _In_ WINBIO_BIR_PURPOSE Purpose,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    )
{
    DebugLog("Called EngineAdapterAcceptSampleData\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(SampleBuffer) ||
        !ARGUMENT_PRESENT(RejectDetail))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    context->LastMKBPValue = 0;

    // Verify that input arguments are valid.
    if (SampleSize < sizeof(UINT32) ||
        Purpose == WINBIO_NO_PURPOSE_AVAILABLE)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    UINT32 MKBPValue = *(UINT32*)SampleBuffer;
    DebugLog("[Engine] MKBP Value: 0x%x\n", MKBPValue);
    context->LastMKBPValue = MKBPValue;

    *RejectDetail = 0;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterExportEngineData(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIR_DATA_FLAGS Flags,
    _Out_ PWINBIO_BIR *SampleBuffer,
    _Out_ PSIZE_T SampleSize
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(SampleBuffer);
    UNREFERENCED_PARAMETER(SampleSize);

    DebugLog("Called EngineAdapterExportEngineData\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterVerifyFeatureSet(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _Out_ PBOOLEAN Match,
    _Out_ PUCHAR *PayloadBlob,
    _Out_ PSIZE_T PayloadBlobSize,
    _Out_ PUCHAR *HashValue,
    _Out_ PSIZE_T HashSize,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(Identity);
    UNREFERENCED_PARAMETER(SubFactor);
    UNREFERENCED_PARAMETER(Match);
    UNREFERENCED_PARAMETER(PayloadBlob);
    UNREFERENCED_PARAMETER(PayloadBlobSize);
    UNREFERENCED_PARAMETER(HashValue);
    UNREFERENCED_PARAMETER(HashSize);
    UNREFERENCED_PARAMETER(RejectDetail);

    DebugLog("Called EngineAdapterVerifyFeatureSet\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterIdentifyFeatureSet(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_IDENTITY Identity,
    _Out_ PWINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _Out_ PUCHAR *PayloadBlob,
    _Out_ PSIZE_T PayloadBlobSize,
    _Out_ PUCHAR *HashValue,
    _Out_ PSIZE_T HashSize,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    )
{
    DebugLog("Called EngineAdapterIdentifyFeatureSet\n");

    HRESULT hr = S_OK;
    SIZE_T recordCount = 0;
    WINBIO_STORAGE_RECORD thisRecord;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(Identity) ||
        !ARGUMENT_PRESENT(SubFactor) ||
        !ARGUMENT_PRESENT(PayloadBlob) ||
        !ARGUMENT_PRESENT(PayloadBlobSize) ||
        !ARGUMENT_PRESENT(HashValue) ||
        !ARGUMENT_PRESENT(HashSize) ||
        !ARGUMENT_PRESENT(RejectDetail))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    // Initialize the return values.
    ZeroMemory(Identity, sizeof(WINBIO_IDENTITY));
    Identity->Type = WINBIO_ID_TYPE_NULL;
    *SubFactor = WINBIO_SUBTYPE_NO_INFORMATION;
    *PayloadBlob = NULL;
    *PayloadBlobSize = 0;
    *HashValue = NULL;
    *HashSize = 0;
    *RejectDetail = 0;

    // Determine the size of the result set. WbioStorageGetRecordCount is a wrapper
    // function in the Winbio_adapter.h header file.
    hr = WbioStorageGetRecordCount(Pipeline, &recordCount);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    UINT8 matchIdx = (EC_MKBP_FP_MATCH_IDX_MASK >> EC_MKBP_FP_MATCH_IDX_OFFSET);
    if (context->LastMKBPValue & EC_MKBP_FP_MATCH) {
        matchIdx = EC_MKBP_FP_MATCH_IDX(context->LastMKBPValue);
    }
    DebugLog("Match Index: %d\n", matchIdx);

    if (matchIdx == (EC_MKBP_FP_MATCH_IDX_MASK >> EC_MKBP_FP_MATCH_IDX_OFFSET)) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
        DebugLog("Invalid Finger\n", matchIdx);
        goto cleanup;
    }

    if (matchIdx >= recordCount) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
        DebugLog("Finger not in database??\n", matchIdx);
        goto cleanup;
    }

    // Point the result set cursor at the first record. WbioStorageFirstRecord
    // is a wrapper function in the Winbio_adapter.h header file.
    hr = WbioStorageFirstRecord(Pipeline);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    for (UINT8 i = 0; i < matchIdx; i++) {
        hr = WbioStorageNextRecord(Pipeline);
        if (FAILED(hr))
        {
            goto cleanup;
        }
    }

    hr = WbioStorageGetCurrentRecord(Pipeline, &thisRecord);
    if (FAILED(hr))
    {
        DebugLog("[Engine] Failed to get record\n");
        goto cleanup;
    }

    DebugLog("[Engine] Got identity type %d\n", thisRecord.Identity->Type);

    if (EC_MKBP_FP_ERRCODE(context->LastMKBPValue) == EC_MKBP_FP_ERR_MATCH_YES_UPDATED) {
        DebugLog("[Engine] Need to update template %d\n", thisRecord.IndexVector[0]);

        PUCHAR newBuffer;
        if (SUCCEEDED(DownloadTemplate(Pipeline, &newBuffer, (UINT32)thisRecord.TemplateBlobSize, thisRecord.IndexVector[0]))) {
            DebugLog("Success!\n");
            RtlCopyMemory(thisRecord.TemplateBlob, newBuffer, thisRecord.TemplateBlobSize);
            free(newBuffer);

            SIZE_T ReceivedSz;
            ULONG Status;
            WbioStorageControlUnit(Pipeline,
                StorageControlCodeSaveToDisk,
                NULL, 0,
                NULL, 0,
                &ReceivedSz,
                &Status);
        }
    }

    // Return information about the matching template to the caller.
    CopyMemory(Identity, thisRecord.Identity, sizeof(WINBIO_IDENTITY));

    *SubFactor = thisRecord.SubFactor;
    *PayloadBlob = thisRecord.PayloadBlob;
    *PayloadBlobSize = thisRecord.PayloadBlobSize;

cleanup:
    if (hr == WINBIO_E_DATABASE_NO_RESULTS)
    {
        hr = WINBIO_E_UNKNOWN_ID;
    }
    DebugLog("[Engine] Returning match result 0x%x\n", hr);
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterCreateEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called EngineAdapterCreateEnrollment\n");
    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }


    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    // Return if an enrollment is already in progress. This example assumes that 
    // your engine adapter context contains an enrollment object.
    if (context->Enrollment.InProgress == TRUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    // Initialize any Enrollment data members not initialized by  the 
    // _AdapterCreateEnrollmentTemplate function. This example assumes that
    // your enrollment object contains at a minimum a field that specifies 
    // the number of biometric samples and another that specifies whether a
    // new enrollment is in progress.
    context->Enrollment.EnrollmentProgress = 0;
    context->Enrollment.InProgress = TRUE;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterUpdateEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    )
{
    DebugLog("Called EngineAdapterUpdateEnrollment\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(RejectDetail))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    // Return if an enrollment is not in progress. This example assumes that 
    // your engine adapter context contains an enrollment object.
    if (context->Enrollment.InProgress != TRUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    *RejectDetail = 0;

    if (context->LastMKBPValue & EC_MKBP_FP_ENROLL)
        context->Enrollment.EnrollmentProgress = EC_MKBP_FP_ENROLL_PROGRESS(context->LastMKBPValue);
    if (context->Enrollment.EnrollmentProgress >= 100) {
        DebugLog("Returning Enrollment complete\n");
        hr = S_OK;
    }
    else {
        DebugLog("Returning Enrollment need more data\n");
        hr = WINBIO_I_MORE_DATA;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterGetEnrollmentStatus(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_REJECT_DETAIL RejectDetail
    )
{
    DebugLog("Called EngineAdapterGetEnrollmentStatus\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(RejectDetail))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    // Return if an enrollment is not in progress. This example assumes that 
    // your engine adapter context contains an enrollment object.
    if (context->Enrollment.InProgress != TRUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    *RejectDetail = 0;

    if (context->Enrollment.EnrollmentProgress >= 100) {
        hr = S_OK;
    }
    else {
        hr = WINBIO_I_MORE_DATA;
    }

 cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterGetEnrollmentHash(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PUCHAR *HashValue,
    _Out_ PSIZE_T HashSize
    )
{
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(HashValue);
    UNREFERENCED_PARAMETER(HashSize);

    DebugLog("Called EngineAdapterGetEnrollmentHash\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterCheckForDuplicate(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_IDENTITY Identity,
    _Out_ PWINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _Out_ PBOOLEAN Duplicate
    )
{
    DebugLog("Called EngineAdapterCheckForDuplicate\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(Identity) ||
        !ARGUMENT_PRESENT(SubFactor) ||
        !ARGUMENT_PRESENT(Duplicate))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Zero the memory pointed to by the Identity argument and set the
    // pointer to NULL.
    ZeroMemory(Identity, sizeof(WINBIO_IDENTITY));
    Identity->Type = WINBIO_ID_TYPE_NULL;

    // Eliminate sub-factor information.
    *SubFactor = WINBIO_SUBTYPE_NO_INFORMATION;

    // Initialize the Boolean Duplicate argument to FALSE.
    *Duplicate = FALSE;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterCommitEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _In_ PUCHAR PayloadBlob,
    _In_ SIZE_T PayloadBlobSize
    )
{
    UNREFERENCED_PARAMETER(PayloadBlob);
    UNREFERENCED_PARAMETER(PayloadBlobSize);

    DebugLog("Called EngineAdapterCommitEnrollment\n");

    HRESULT hr = S_OK;
    WINBIO_STORAGE_RECORD newTemplate = { 0 };

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(Identity))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    // Return if an enrollment is not in progress. This example assumes that 
    // an enrollment object is part of your engine context structure.
    if (context->Enrollment.InProgress != TRUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    newTemplate.Identity = Identity;
    newTemplate.SubFactor = SubFactor;
    newTemplate.IndexVector = NULL;
    newTemplate.IndexElementCount = 0;
    newTemplate.TemplateBlob = NULL;
    newTemplate.TemplateBlobSize = 0;
    newTemplate.PayloadBlob = PayloadBlob;
    newTemplate.PayloadBlobSize = PayloadBlobSize;

    hr = WbioStorageAddRecord(
        Pipeline,
        &newTemplate
    );

    if (FAILED(hr))
    {
        goto cleanup;
    }

    // Specify that the enrollment process has been completed.
    context->Enrollment.InProgress = FALSE;
    
cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterDiscardEnrollment(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called EngineAdapterDiscardEnrollment\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_ENGINE_CONTEXT context =
        (PWINIBIO_ENGINE_CONTEXT)Pipeline->EngineContext;

    // Return if an enrollment is not in progress. This example assumes that 
    // an enrollment object is part of your engine context structure.
    if (context->Enrollment.InProgress != TRUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    // If the _AdapterDestroyEnrollmentTemplate function does not reset the
    // InProgress data member, reset it here.
    context->Enrollment.InProgress = FALSE;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterControlUnit(
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

    DebugLog("Called EngineAdapterControlUnit\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
EngineAdapterControlUnitPrivileged(
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

    DebugLog("Called EngineAdapterControlUnitPrivileged\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------

