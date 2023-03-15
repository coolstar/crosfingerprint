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
#include "../crosfingerprint/ec_commands.h"


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
    {0x7f6c2610, 0xfdba, 0x41a3, {0xae, 0x1c, 0x8f, 0xd5, 0x84, 0x59, 0x8d, 0x13}},

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

    PWINIBIO_STORAGE_CONTEXT newContext = (PWINIBIO_STORAGE_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINIBIO_STORAGE_CONTEXT));
    if (!newContext) {
        return E_OUTOFMEMORY;
    }

    Pipeline->StorageContext = newContext;

    struct ec_response_get_version r;
    hr = ec_command(Pipeline->SensorHandle, EC_CMD_GET_VERSION, 0, NULL, 0, &r, sizeof(struct ec_response_get_version));

    if (!FAILED(hr)) {
        /* Ensure versions are null-terminated before we print them */
        r.version_string_ro[sizeof(r.version_string_ro) - 1] = '\0';
        r.version_string_rw[sizeof(r.version_string_rw) - 1] = '\0';

        DebugLog("EC RO Version: %s\n", r.version_string_ro);
        DebugLog("EC RW Version: %s\n", r.version_string_rw);
    }
    else {
        DebugLog("Error: Could not get version\n");
    }

    struct ec_response_fp_info info;
    for (int tries = 1; tries <= 10; tries++) {
        hr = ec_command(Pipeline->SensorHandle, EC_CMD_FP_INFO, 1, NULL, 0, &info, sizeof(struct ec_response_fp_info));
        if (FAILED(hr)) {
            Sleep(500);
            DebugLog("Failed to get FP info; Retrying (%d of 10)...\n", tries);
        }
        else {
            break;
        }
    }

    if (FAILED(hr)) {
        goto cleanup;
    }

    DebugLog("Max Templates: %d, Template Size: %d, Valid Templates: %d, Template Version: %d\n", info.template_max, info.template_size, info.template_valid, info.template_version);

    newContext->MaxFingers = info.template_max;
    newContext->TemplateSize = info.template_size;

    DebugLog("Handles: sensor 0x%lx, engine 0x%lx, storage 0x%lx\n", Pipeline->SensorHandle, Pipeline->EngineHandle, Pipeline->StorageHandle);

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

    if (Pipeline->StorageContext) {
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, Pipeline->StorageContext);
        Pipeline->StorageContext = NULL;
    }

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
    UNREFERENCED_PARAMETER(Pipeline);

    DebugLog("Called StorageAdapterClearContext\n");

    return S_OK;
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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(DatabaseId);
    UNREFERENCED_PARAMETER(Factor);
    UNREFERENCED_PARAMETER(Format);
    UNREFERENCED_PARAMETER(FilePath);
    UNREFERENCED_PARAMETER(ConnectString);
    UNREFERENCED_PARAMETER(IndexElementCount);
    UNREFERENCED_PARAMETER(InitialSize);

    UNREFERENCED_PARAMETER(Pipeline);

    DebugLog("Called StorageAdapterCreateDatabase. File Path: %ls\n", FilePath);

    return S_OK;
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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(DatabaseId);
    UNREFERENCED_PARAMETER(FilePath);
    UNREFERENCED_PARAMETER(ConnectString);

    DebugLog("Called StorageAdapterEraseDatabase. File Path: %ls\n", FilePath);

    return S_OK;
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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(DatabaseId);
    UNREFERENCED_PARAMETER(FilePath);
    UNREFERENCED_PARAMETER(ConnectString);

    DebugLog("Called StorageAdapterOpenDatabase. File Path: %ls\n", FilePath);

    return S_OK;
}
//-----------------------------------------------------------------------------

static HRESULT
WINAPI
StorageAdapterCloseDatabase(
    _Inout_ PWINBIO_PIPELINE Pipeline
    )
{
    UNREFERENCED_PARAMETER(Pipeline);

    DebugLog("Called StorageAdapterCloseDatabase\n");

    return S_OK;
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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(Format);
    UNREFERENCED_PARAMETER(Version);

    DebugLog("Called StorageAdapterGetDataFormat\n");

    return E_NOTIMPL;
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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(AvailableRecordCount);
    UNREFERENCED_PARAMETER(TotalRecordCount);

    DebugLog("Called StorageAdapterGetDatabaseSize\n");

    return E_NOTIMPL;
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

    if (Pipeline->StorageContext->Database.size() == Pipeline->StorageContext->MaxFingers) {
        hr = WINBIO_E_DATABASE_FULL;
        goto cleanup;
    }

    RtlCopyMemory(&NewRecord.Identity, RecordContents->Identity, sizeof(NewRecord.Identity));
    NewRecord.SubFactor = RecordContents->SubFactor;
    Pipeline->StorageContext->Database.push_back(NewRecord);

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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(Identity);
    UNREFERENCED_PARAMETER(SubFactor);

    DebugLog("Called StorageAdapterDeleteRecord. Identity Type %d\n", Identity->Type);

    return S_OK;
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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(Identity);
    UNREFERENCED_PARAMETER(SubFactor);

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
    if (storageContext == NULL /*|| storageContext->FileHandle == INVALID_HANDLE_VALUE*/)
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

    DebugLog("Identity Type: %d, Subfactor: %d\n", Identity->Type, SubFactor);
    if (Identity->Type == WINBIO_ID_TYPE_WILDCARD) {
        hr = S_OK;
        goto cleanup;
    }

    if (Identity->Type == WINBIO_ID_TYPE_GUID) {
        DebugLog("GUID: {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
            Identity->Value.TemplateGuid.Data1, Identity->Value.TemplateGuid.Data2, Identity->Value.TemplateGuid.Data3,
            Identity->Value.TemplateGuid.Data4[0], Identity->Value.TemplateGuid.Data4[1], Identity->Value.TemplateGuid.Data4[2], Identity->Value.TemplateGuid.Data4[3],
            Identity->Value.TemplateGuid.Data4[4], Identity->Value.TemplateGuid.Data4[5], Identity->Value.TemplateGuid.Data4[6], Identity->Value.TemplateGuid.Data4[7]);
    }

    if (Identity->Type == WINBIO_ID_TYPE_SID) {
        DebugLog("Identity SID size %d: ", Identity->Value.AccountSid.Size);
        for (int j = 0; j < max(Identity->Value.AccountSid.Size, sizeof(Identity->Value.AccountSid.Data)); j++) {
            DebugLog("0x%x ", Identity->Value.AccountSid.Data[j]);
        }
        DebugLog("\n");
    }

    hr = WINBIO_E_DATABASE_NO_RESULTS;
    for (int i = 0; i < storageContext->Database.size(); i++) {
        CRFP_STORAGE_RECORD record = storageContext->Database[i];

        DebugLog("Record %d: \n", i);

        DebugLog("\tIdentity Type: %d, Subfactor: %d\n", record.Identity.Type, record.SubFactor);
        if (record.Identity.Type == WINBIO_ID_TYPE_GUID) {
            DebugLog("GUID: {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
                record.Identity.Value.TemplateGuid.Data1, record.Identity.Value.TemplateGuid.Data2, record.Identity.Value.TemplateGuid.Data3,
                record.Identity.Value.TemplateGuid.Data4[0], record.Identity.Value.TemplateGuid.Data4[1], record.Identity.Value.TemplateGuid.Data4[2], record.Identity.Value.TemplateGuid.Data4[3],
                record.Identity.Value.TemplateGuid.Data4[4], record.Identity.Value.TemplateGuid.Data4[5], record.Identity.Value.TemplateGuid.Data4[6], record.Identity.Value.TemplateGuid.Data4[7]);
        }

        if (record.Identity.Type == WINBIO_ID_TYPE_SID) {
            DebugLog("\tIdentity SID size %d: ", record.Identity.Value.AccountSid.Size);
            for (int j = 0; j < max(record.Identity.Value.AccountSid.Size, sizeof(record.Identity.Value.AccountSid.Data)); j++) {
                DebugLog("0x%x ", record.Identity.Value.AccountSid.Data[j]);
            }
            DebugLog("\n"); 
        }

        if ((SubFactor == WINBIO_SUBTYPE_ANY || SubFactor == record.SubFactor) && 
            Identity->Type == record.Identity.Type) {
            if (record.Identity.Type == WINBIO_ID_TYPE_GUID &&
                (memcmp(&Identity->Value.TemplateGuid, &record.Identity.Value.TemplateGuid, sizeof(Identity->Value.TemplateGuid)) == 0)) {
                hr = S_OK;
                break;
            }
            else if (record.Identity.Type == WINBIO_ID_TYPE_SID &&
                Identity->Value.AccountSid.Size == record.Identity.Value.AccountSid.Size &&
                Identity->Value.AccountSid.Size <= sizeof(Identity->Value.AccountSid.Data) &&
                (memcmp(Identity->Value.AccountSid.Data, record.Identity.Value.AccountSid.Data, Identity->Value.AccountSid.Size) == 0)){
                hr = S_OK;
                break;
            }
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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(SubFactor);
    UNREFERENCED_PARAMETER(IndexVector);
    UNREFERENCED_PARAMETER(IndexElementCount);

    DebugLog("Called StorageAdapterQueryByContent\n");

    return E_NOTIMPL;
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

    *RecordCount = Pipeline->StorageContext->Database.size();

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

    Pipeline->StorageContext->DatabaseCursor = 0;

    if (Pipeline->StorageContext->Database.empty()) {
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

    if (Pipeline->StorageContext->Database.empty()) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
    } else if (Pipeline->StorageContext->DatabaseCursor >= Pipeline->StorageContext->Database.size() - 1) {
        hr = WINBIO_E_DATABASE_NO_MORE_RECORDS;
    }
    else {
        Pipeline->StorageContext->DatabaseCursor++;
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

    if (Pipeline->StorageContext->Database.empty()) {
        hr = WINBIO_E_DATABASE_NO_RESULTS;
        goto cleanup;
    }

    if (Pipeline->StorageContext->DatabaseCursor > Pipeline->StorageContext->Database.size()) {
        Pipeline->StorageContext->DatabaseCursor = Pipeline->StorageContext->Database.size() - 1;
    }

    PCRFP_STORAGE_RECORD record = &Pipeline->StorageContext->Database[Pipeline->StorageContext->DatabaseCursor];

    DebugLog("[Storage] Got identity type %d\n", record->Identity.Type);

    RtlZeroMemory(RecordContents, sizeof(*RecordContents));
    RecordContents->Identity = &record->Identity;
    RecordContents->SubFactor = record->SubFactor;

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
    UNREFERENCED_PARAMETER(Pipeline);
    UNREFERENCED_PARAMETER(ControlCode);
    UNREFERENCED_PARAMETER(SendBuffer);
    UNREFERENCED_PARAMETER(SendBufferSize);
    UNREFERENCED_PARAMETER(ReceiveBuffer);
    UNREFERENCED_PARAMETER(ReceiveBufferSize);
    UNREFERENCED_PARAMETER(ReceiveDataSize);
    UNREFERENCED_PARAMETER(OperationStatus);

    DebugLog("Called StorageAdapterControlUnit\n");

    return E_NOTIMPL;
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


