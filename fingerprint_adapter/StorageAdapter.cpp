/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    Storage.cpp

Abstract:

    This module contains a stub implementation of a Storage Adapter
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
#include "StorageAdapter.h"

///////////////////////////////////////////////////////////////////////////////
//
// Forward declarations for the Storage Adapter's interface routines...
//
///////////////////////////////////////////////////////////////////////////////
static HRESULT
WINAPI
StorageAdapterAttach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
StorageAdapterDetach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
StorageAdapterClearContext(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
StorageAdapterCreateDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_UUID DatabaseId,
    _In_ WINBIO_BIOMETRIC_TYPE Factor,
    _In_ PWINBIO_UUID Format,
    _In_ LPCWSTR FilePath,
    _In_ LPCWSTR ConnectString,
    _In_ SIZE_T IndexElementCount,
    _In_ SIZE_T InitialSize
    );

static HRESULT
WINAPI
StorageAdapterEraseDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_UUID DatabaseId,
    _In_ LPCWSTR FilePath,
    _In_ LPCWSTR ConnectString
    );

static HRESULT
WINAPI
StorageAdapterOpenDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_UUID DatabaseId,
    _In_ LPCWSTR FilePath,
    _In_ LPCWSTR ConnectString
    );

static HRESULT
WINAPI
StorageAdapterCloseDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
StorageAdapterGetDataFormat(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_UUID Format,
    _Out_ PWINBIO_VERSION Version
    );

static HRESULT
WINAPI
StorageAdapterGetDatabaseSize(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T AvailableRecordCount,
    _Out_ PSIZE_T TotalRecordCount
    );

static HRESULT
WINAPI
StorageAdapterAddRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_STORAGE_RECORD RecordContents
    );

static HRESULT
WINAPI
StorageAdapterDeleteRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor
    );

static HRESULT
WINAPI
StorageAdapterQueryBySubject(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor
    );

static HRESULT
WINAPI
StorageAdapterQueryByContent(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _In_ ULONG IndexVector[],
    _In_ SIZE_T IndexElementCount
    );

static HRESULT
WINAPI
StorageAdapterGetRecordCount(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T RecordCount
    );

static HRESULT
WINAPI
StorageAdapterFirstRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
StorageAdapterNextRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline
    );

static HRESULT
WINAPI
StorageAdapterGetCurrentRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_STORAGE_RECORD RecordContents
    );

static HRESULT
WINAPI
StorageAdapterControlUnit(
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
StorageAdapterControlUnitPrivileged(
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
static WINBIO_STORAGE_INTERFACE g_StorageInterface = {
    WINBIO_STORAGE_INTERFACE_VERSION_1,
    WINBIO_ADAPTER_TYPE_STORAGE,
    sizeof(WINBIO_STORAGE_INTERFACE),
    {0xe1230247, 0x7ebf, 0x4167, {0xb6, 0xbf, 0x31, 0x1f, 0xbc, 0x7e, 0xb6, 0x3b}},

    StorageAdapterAttach,
    StorageAdapterDetach,
    StorageAdapterClearContext,
    StorageAdapterCreateDatabase,
    StorageAdapterEraseDatabase,
    StorageAdapterOpenDatabase,
    StorageAdapterCloseDatabase,
    StorageAdapterGetDataFormat,
    StorageAdapterGetDatabaseSize,
    StorageAdapterAddRecord,
    StorageAdapterDeleteRecord,
    StorageAdapterQueryBySubject,
    StorageAdapterQueryByContent,
    StorageAdapterGetRecordCount,
    StorageAdapterFirstRecord,
    StorageAdapterNextRecord,
    StorageAdapterGetCurrentRecord,
    StorageAdapterControlUnit,
    StorageAdapterControlUnitPrivileged
};
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
// Well-known interface-discovery function exported by the Storage Adapter
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
WbioQueryStorageInterface(
    _Out_ PWINBIO_STORAGE_INTERFACE *StorageInterface
    )
{
    if (!ARGUMENT_PRESENT(StorageInterface)) {
        return E_POINTER;
    }
    *StorageInterface = &g_StorageInterface;
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
StorageAdapterAttach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called StorageAdapterAttach\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->StorageContext != NULL ||
        Pipeline->StorageHandle != INVALID_HANDLE_VALUE) {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    PWINIBIO_STORAGE_CONTEXT newContext = (PWINIBIO_STORAGE_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINIBIO_STORAGE_CONTEXT));
    if (!newContext) {
        return E_OUTOFMEMORY;
    }

    Pipeline->StorageContext = newContext;

    struct ec_response_fp_info info;
    for (int tries = 1; tries <= 10; tries++) {
        hr = ec_command(Pipeline, EC_CMD_FP_INFO, 1, NULL, 0, &info, sizeof(struct ec_response_fp_info));
        if (hr != 0) {
            Sleep(500);
        }
        else {
            break;
        }
    }

    if (hr != 0) {
        DebugLog("Failed to get FP info\n");
        hr = WINBIO_E_DEVICE_FAILURE;
        goto cleanup;
    }

    DebugLog("Max Templates: %d, Template Size: %d, Valid Templates: %d, Template Version: %d\n", info.template_max, info.template_size, info.template_valid, info.template_version);

    newContext->FingerHeight = info.height;
    newContext->FingerWidth = info.width;
    newContext->MaxFingers = info.template_max;
    newContext->TemplateSize = info.template_size;
cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterDetach(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called StorageAdapterDetach\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->StorageContext == NULL) {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    StorageAdapterClearContext(Pipeline);
    StorageAdapterCloseDatabase(Pipeline);

    HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, Pipeline->StorageContext);
    Pipeline->StorageContext = NULL;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterClearContext(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    HRESULT hr = S_OK;

    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    storageContext->RecordCursor = 0;
    storageContext->Records = std::vector<PCRFP_STORAGE_RECORD>();

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterCreateDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_UUID DatabaseId,
    _In_ WINBIO_BIOMETRIC_TYPE Factor,
    _In_ PWINBIO_UUID Format,
    _In_ LPCWSTR FilePath,
    _In_ LPCWSTR ConnectString,
    _In_ SIZE_T IndexElementCount,
    _In_ SIZE_T InitialSize
    )
{
    UNREFERENCED_PARAMETER(Factor);
    UNREFERENCED_PARAMETER(InitialSize);

    DebugLog("Called StorageAdapterCreateDatabase. File Path: %ls\n", FilePath);

    HRESULT hr = S_OK;

    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(FilePath) ||
        !ARGUMENT_PRESENT(DatabaseId) ||
        !ARGUMENT_PRESENT(Format) ||
        !ARGUMENT_PRESENT(ConnectString))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->StorageContext == NULL ||
        Pipeline->StorageHandle != INVALID_HANDLE_VALUE) {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    if (wcscmp(FilePath, L"") == 0) {
        FilePath = L"C:\\Windows\\Temp\\crfptestdb.bin";
    }

    HANDLE databaseFile = CreateFile(
        FilePath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (databaseFile != INVALID_HANDLE_VALUE) {
        hr = WINBIO_E_DATABASE_ALREADY_EXISTS;
        CloseHandle(databaseFile);
        goto cleanup;
    }

    Pipeline->StorageContext->DataFormat = *Format;
    Pipeline->StorageContext->IndexElementCount = IndexElementCount;

    hr = WbioStorageOpenDatabase(Pipeline, DatabaseId, FilePath, ConnectString);

    SIZE_T ReceivedSz;
    ULONG Status;
    WbioStorageControlUnit(Pipeline,
        StorageControlCodeSaveToDisk,
        NULL, 0,
        NULL, 0,
        &ReceivedSz,
        &Status);

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterEraseDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_UUID DatabaseId,
    _In_ LPCWSTR FilePath,
    _In_ LPCWSTR ConnectString
    )
{
    DebugLog("Called StorageAdapterEraseDatabase. File Path: %ls\n", FilePath);

    HRESULT hr = S_OK;

    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(DatabaseId) ||
        !ARGUMENT_PRESENT(FilePath) ||
        !ARGUMENT_PRESENT(ConnectString))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (wcscmp(FilePath, L"") == 0) {
        FilePath = L"C:\\Windows\\Temp\\crfptestdb.bin";
    }

    DebugLog("%s: Context 0x%x handle 0x%x\n", __func__, Pipeline->StorageContext, Pipeline->StorageHandle);
    if (Pipeline->StorageContext == NULL) {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    if (Pipeline->StorageHandle != INVALID_HANDLE_VALUE) {
        WINBIO_IDENTITY Identity = { 0 };
        Identity.Type = WINBIO_ID_TYPE_WILDCARD;
        Identity.Value.Wildcard = WINBIO_IDENTITY_WILDCARD;
        hr = StorageAdapterDeleteRecord(Pipeline, &Identity, WINBIO_SUBTYPE_ANY);
        if (FAILED(hr)) {
            goto cleanup;
        }
    }
    else {
        if (!DeleteFile(FilePath)) {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                hr = WINBIO_E_DATABASE_CANT_ERASE;
            }
        }
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterOpenDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_UUID DatabaseId,
    _In_ LPCWSTR FilePath,
    _In_ LPCWSTR ConnectString
    )
{
    DebugLog("Called StorageAdapterOpenDatabase. File Path: %ls\n", FilePath);

    HRESULT hr = S_OK;

    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(DatabaseId) ||
        !ARGUMENT_PRESENT(FilePath) ||
        !ARGUMENT_PRESENT(ConnectString))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (wcscmp(FilePath, L"") == 0) {
        FilePath = L"C:\\Windows\\Temp\\crfptestdb.bin";
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext = (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    if (Pipeline->StorageHandle != INVALID_HANDLE_VALUE) {
        hr = S_OK;
        goto cleanup;
    }
    else {
        Pipeline->StorageHandle = CreateFile(
            FilePath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (Pipeline->StorageHandle == INVALID_HANDLE_VALUE) {
            hr = WINBIO_E_DATABASE_CANT_OPEN;
            goto cleanup;
        }
    }

    hr = LoadDatabase(Pipeline);
    if (FAILED(hr)) {
        goto cleanup;
    }

    SIZE_T ReceivedSz;
    ULONG Status;
    hr = WbioStorageControlUnit(Pipeline,
        StorageControlCodeUploadToHw,
        NULL, 0,
        NULL, 0,
        &ReceivedSz,
        &Status);
    if (FAILED(hr)) {
        goto cleanup;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterCloseDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called StorageAdapterCloseDatabase\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL ||
        Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    // Close the database file handle.
    CloseHandle(Pipeline->StorageHandle);
    Pipeline->StorageHandle = INVALID_HANDLE_VALUE;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterGetDataFormat(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_UUID Format,
    _Out_ PWINBIO_VERSION Version
    )
{
    DebugLog("Called StorageAdapterGetDataFormat\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(Format) ||
        !ARGUMENT_PRESENT(Version))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    if (storageContext == NULL ||
        Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    *Format = storageContext->DataFormat;

    Version->MajorVersion = 1;
    Version->MinorVersion = 0;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterGetDatabaseSize(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T AvailableRecordCount,
    _Out_ PSIZE_T TotalRecordCount
    )
{
    DebugLog("Called StorageAdapterGetDatabaseSize\n");

    HRESULT hr = S_OK;

    // Verify that the Pipeline parameter is not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(AvailableRecordCount) ||
        !ARGUMENT_PRESENT(TotalRecordCount))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL ||
        Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    *TotalRecordCount = storageContext->Database.size();
    *AvailableRecordCount = storageContext->MaxFingers - *TotalRecordCount;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterAddRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_STORAGE_RECORD RecordContents
    )
{
    DebugLog("Called StorageAdapterAddRecord\n");

    HRESULT hr = S_OK;
    CRFP_STORAGE_RECORD NewRecord = { 0 };

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(RecordContents))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext = (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL || Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    if (RecordContents->Identity->Type != WINBIO_ID_TYPE_GUID && RecordContents->Identity->Type != WINBIO_ID_TYPE_SID) {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    if (RecordContents->SubFactor == WINBIO_SUBTYPE_NO_INFORMATION || RecordContents->SubFactor == WINBIO_SUBTYPE_ANY) {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    if (RecordContents->IndexElementCount != storageContext->IndexElementCount) {
        hr = WINBIO_E_DATABASE_BAD_INDEX_VECTOR;
        goto cleanup;
    }
    if (storageContext->IndexElementCount > 0 &&
        !ARGUMENT_PRESENT(RecordContents->IndexVector))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (RecordContents->TemplateBlob == NULL || RecordContents->TemplateBlobSize == 0) {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    if (storageContext->Database.size() == storageContext->MaxFingers) {
        hr = WINBIO_E_DATABASE_FULL;
        goto cleanup;
    }

    WbioStorageClearContext(Pipeline);

    NewRecord.StorageRecordSize = sizeof(NewRecord);

    RtlCopyMemory(&NewRecord.Identity, RecordContents->Identity, sizeof(NewRecord.Identity));
    NewRecord.SubFactor = RecordContents->SubFactor;

    NewRecord.TemplateData = RecordContents->TemplateBlob;
    NewRecord.TemplateSize = RecordContents->TemplateBlobSize;

    storageContext->Database.push_back(NewRecord);

    SIZE_T ReceivedSz;
    ULONG Status;
    hr = WbioStorageControlUnit(Pipeline,
        StorageControlCodeSaveToDisk,
        NULL, 0,
        NULL, 0,
        &ReceivedSz,
        &Status);
    if (FAILED(hr)) {
        hr = WINBIO_E_DATABASE_WRITE_ERROR;
        goto cleanup;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterDeleteRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor
    )
{
    DebugLog("Called StorageAdapterDeleteRecord. Identity Type %d\n", Identity->Type);

    HRESULT hr = S_OK;
    std::vector<CRFP_STORAGE_RECORD>::iterator it;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext = (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL || Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    // Verify the Identity argument.
    if (Identity->Type != WINBIO_ID_TYPE_GUID &&
        Identity->Type != WINBIO_ID_TYPE_SID &&
        Identity->Type != WINBIO_ID_TYPE_WILDCARD)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    if (Identity->Type == WINBIO_ID_TYPE_WILDCARD &&
        Identity->Value.Wildcard != WINBIO_IDENTITY_WILDCARD)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    // WINBIO_SUBTYPE_ANY is a valid sub-factor.
    // WINBIO_SUBTYPE_NO_INFORMATION is not a valid sub-factor.
    if (SubFactor == WINBIO_SUBTYPE_NO_INFORMATION)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    WbioStorageClearContext(Pipeline);

    BOOLEAN foundMatch = FALSE;

    it = storageContext->Database.begin();
    while (it != storageContext->Database.end()) {
        if (MatchSubject(Identity, SubFactor, *it)) {
            DebugLog("Matched for deletion!\n");
            foundMatch = TRUE;
            it = storageContext->Database.erase(it);
        }
        else {
            ++it;
        }
    }

    if (!foundMatch) {
        hr = WINBIO_E_DATABASE_NO_SUCH_RECORD;
        goto cleanup;
    }

    DebugLog("New Count: %d\n", storageContext->Database.size());

    SIZE_T ReceivedSz;
    ULONG Status;
    hr = WbioStorageControlUnit(Pipeline,
        StorageControlCodeSaveToDisk,
        NULL, 0,
        NULL, 0,
        &ReceivedSz,
        &Status);
    if (FAILED(hr)) {
        goto cleanup;
    }

    hr = WbioStorageControlUnit(Pipeline,
        StorageControlCodeUploadToHw,
        NULL, 0,
        NULL, 0,
        &ReceivedSz,
        &Status);
    if (FAILED(hr)) {
        goto cleanup;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterQueryBySubject(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor
    )
{
    DebugLog("Called StorageAdapterQueryBySubject. Identity Type %d\n", Identity->Type);

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext = (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL || Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    // Verify the Identity argument.
    if (Identity->Type != WINBIO_ID_TYPE_GUID &&
        Identity->Type != WINBIO_ID_TYPE_SID &&
        Identity->Type != WINBIO_ID_TYPE_WILDCARD)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    if (Identity->Type == WINBIO_ID_TYPE_WILDCARD &&
        Identity->Value.Wildcard != WINBIO_IDENTITY_WILDCARD)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    // WINBIO_SUBTYPE_ANY is a valid sub-factor.
    // WINBIO_SUBTYPE_NO_INFORMATION is not a valid sub-factor.
    if (SubFactor == WINBIO_SUBTYPE_NO_INFORMATION)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    
    if (storageContext->Database.empty()) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
        goto cleanup;
    }

    WbioStorageClearContext(Pipeline);

    DebugLog("Identity Type: %d, Subfactor: %d\n", Identity->Type, SubFactor);
    if (Identity->Type == WINBIO_ID_TYPE_WILDCARD) {
        hr = S_OK;

        for (int i = 0; i < storageContext->Database.size(); i++) {
            CRFP_STORAGE_RECORD& record = storageContext->Database[i];
            storageContext->Records.push_back(&storageContext->Database[i]);
        }

        goto cleanup;
    }

    hr = WINBIO_E_DATABASE_NO_RESULTS;
    for (int i = 0; i < storageContext->Database.size(); i++) {
        CRFP_STORAGE_RECORD &record = storageContext->Database[i];

        DebugLog("Record %d: \n", i);

        DebugLog("\tIdentity Type: %d, Subfactor: %d\n", record.Identity.Type, record.SubFactor);

        if (MatchSubject(Identity, SubFactor, record)) {
            storageContext->Records.push_back(&storageContext->Database[i]);
            hr = S_OK;
        }
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterQueryByContent(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    _In_ ULONG IndexVector[],
    _In_ SIZE_T IndexElementCount
    )
{
    UNREFERENCED_PARAMETER(IndexVector);
    UNREFERENCED_PARAMETER(IndexElementCount);

    DebugLog("Called StorageAdapterQueryByContent\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext = (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the pipeline state.
    if (storageContext == NULL || Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    // WINBIO_SUBTYPE_ANY is a valid sub-factor.
    // WINBIO_SUBTYPE_NO_INFORMATION is not a valid sub-factor.
    if (SubFactor == WINBIO_SUBTYPE_NO_INFORMATION)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    WbioStorageClearContext(Pipeline);

    for (int i = 0; i < storageContext->Database.size(); i++) {
        CRFP_STORAGE_RECORD& record = storageContext->Database[i];
        if (record.SubFactor == SubFactor || SubFactor == WINBIO_SUBTYPE_ANY)
            storageContext->Records.push_back(&storageContext->Database[i]);
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterGetRecordCount(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PSIZE_T RecordCount
    )
{
    DebugLog("Called StorageAdapterGetRecordCount\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) || 
        !ARGUMENT_PRESENT(RecordCount))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->StorageContext->Records.empty()) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
    }

    *RecordCount = Pipeline->StorageContext->Records.size();

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterFirstRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called StorageAdapterFirstRecord\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    Pipeline->StorageContext->RecordCursor = 0;

    if (Pipeline->StorageContext->Records.empty()) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterNextRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    DebugLog("Called StorageAdapterNextRecord\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->StorageContext->Records.empty()) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
    } else if (Pipeline->StorageContext->RecordCursor >= Pipeline->StorageContext->Records.size() - 1) {
        hr = WINBIO_E_DATABASE_NO_MORE_RECORDS;
    }
    else {
        Pipeline->StorageContext->RecordCursor++;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterGetCurrentRecord(
    _Inout_ PWINBIO_PIPELINE Pipeline,
    _Out_ PWINBIO_STORAGE_RECORD RecordContents
    )
{
    DebugLog("Called StorageAdapterGetCurrentRecord\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if (Pipeline->StorageContext->Records.empty()) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
        goto cleanup;
    }

    if (Pipeline->StorageContext->RecordCursor > Pipeline->StorageContext->Records.size()) {
        Pipeline->StorageContext->RecordCursor = (ULONG)(Pipeline->StorageContext->Records.size() - 1);
    }

    PCRFP_STORAGE_RECORD record = Pipeline->StorageContext->Records[Pipeline->StorageContext->RecordCursor];

    DebugLog("[Storage] Got identity type %d\n", record->Identity.Type);

    RtlZeroMemory(RecordContents, sizeof(*RecordContents));
    RecordContents->Identity = &record->Identity;
    RecordContents->SubFactor = record->SubFactor;
    RecordContents->IndexVector = &Pipeline->StorageContext->RecordCursor;
    RecordContents->IndexElementCount = Pipeline->StorageContext->IndexElementCount;
    RecordContents->TemplateBlob = record->TemplateData;
    RecordContents->TemplateBlobSize = record->TemplateSize;

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterControlUnit(
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
    UNREFERENCED_PARAMETER(SendBuffer);
    UNREFERENCED_PARAMETER(SendBufferSize);
    UNREFERENCED_PARAMETER(ReceiveBuffer);
    UNREFERENCED_PARAMETER(ReceiveBufferSize);

    DebugLog("Called StorageAdapterControlUnit\n");

    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline) ||
        !ARGUMENT_PRESENT(ReceiveDataSize) ||
        !ARGUMENT_PRESENT(OperationStatus))
    {
        hr = E_POINTER;
        goto cleanup;
    }

    *ReceiveDataSize = 0;
    *OperationStatus = 0;

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the state of the pipeline.
    if (storageContext == NULL  ||
        Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        goto cleanup;
    }

    switch (ControlCode) {
    case StorageControlCodeUploadToHw:
        DebugLog("Upload to Hardware\n");
        hr = SyncDatabaseToMCU(Pipeline);
        break;
    case StorageControlCodeSaveToDisk:
        DebugLog("Save to disk\n");
        hr = SaveDatabase(Pipeline);
        break;
    case StorageControlDownloadTemplate:
        DebugLog("Download Template\n");

        if (storageContext->Database.size() == storageContext->MaxFingers) {
            hr = WINBIO_E_DATABASE_FULL;
            goto cleanup;
        }

        *ReceiveDataSize = storageContext->TemplateSize;
        hr = DownloadTemplate(Pipeline, (PUCHAR *)ReceiveBuffer, storageContext->TemplateSize, (int)storageContext->Database.size());
        if (FAILED(hr)) {
            goto cleanup;
        }

        break;
    default:
        hr = WINBIO_E_INVALID_CONTROL_CODE;
        break;
    }

cleanup:
    return hr;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterControlUnitPrivileged(
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

    DebugLog("Called StorageAdapterControlUnitPrivileged\n");

    return E_NOTIMPL;
}
//-----------------------------------------------------------------------------


