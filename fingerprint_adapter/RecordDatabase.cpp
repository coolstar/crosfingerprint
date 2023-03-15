#include "precomp.h"
#include "winbio_adapter.h"
#include "StorageAdapter.h"

HRESULT LoadDatabase(PWINBIO_PIPELINE Pipeline) {
	HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        return hr;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the state of the pipeline.
    if (storageContext == NULL ||
        Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        return hr;
    }

    CRFP_DATABASE_FILE_HEADER fileHeader;
    std::vector<CRFP_STORAGE_RECORD> newDatabase;
    UINT32 i;
    DWORD bytesRead;

    if (!ReadFile(Pipeline->StorageHandle,
        &fileHeader, sizeof(CRFP_DATABASE_FILE_HEADER),
        &bytesRead, NULL) || bytesRead != sizeof(CRFP_DATABASE_FILE_HEADER)){
        DebugLog("DB missing header!\n");
        hr = S_OK; //File is probably empty
        goto cleanup;
    }

    if (fileHeader.FileMagic != CRFP_DB_MAGIC) {
        DebugLog("Invalid DB file magic\n");
        hr = S_OK; //Don't bother reading. Treat as empty.
        goto cleanup;
    }

    if (fileHeader.RecordsCount > storageContext->MaxFingers) {
        fileHeader.RecordsCount = storageContext->MaxFingers; //We can fix too many records
    }

    for (i = 0; i < fileHeader.RecordsCount; i++) {
        DebugLog("Loading record %d\n", i);
        CRFP_STORAGE_RECORD newRecord = { 0 };
        DWORD recordSize = min(sizeof(CRFP_STORAGE_RECORD), fileHeader.RecordSize);
        if (!ReadFile(Pipeline->StorageHandle, &newRecord,
            recordSize, &bytesRead, NULL) ||
            bytesRead != recordSize) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugLog("Error loading record 0x%x\n", hr);
            goto cleanup;
        }

        if (newRecord.StorageRecordSize != fileHeader.RecordSize ||
            newRecord.TemplateSize != fileHeader.TemplateSize) {
            hr = WINBIO_E_DATABASE_CORRUPTED;
            DebugLog("Record %d corrupted\n", i);
            goto cleanup;
        }

        newRecord.TemplateData = NULL;
        newRecord.StorageRecordSize = sizeof(newRecord);

        newDatabase.push_back(newRecord);
    }

    for (i = 0; i < fileHeader.RecordsCount; i++) {
        DebugLog("Loading template for record %d\n", i);
        PCRFP_STORAGE_RECORD record = &newDatabase[i];

        record->TemplateData = (PUCHAR)malloc(record->TemplateSize);
        if (!ReadFile(Pipeline->StorageHandle, record->TemplateData,
            record->TemplateSize,
            &bytesRead, NULL) || bytesRead != record->TemplateSize) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugLog("Error loading template 0x%x\n", hr);
            goto cleanup;
        }

        /*struct ec_fp_template_encryption_metadata* metadata = (struct ec_fp_template_encryption_metadata*)record->TemplateData;
        DebugLog("Template version: %d\n", metadata->struct_version);*/
    }

cleanup:
    if (!SUCCEEDED(hr)) {
        for (i = 0; i < newDatabase.size(); i++) {
            if (newDatabase[i].TemplateData)
                free(newDatabase[i].TemplateData);
        }
    }
    storageContext->Database = newDatabase;

    hr = S_OK; //always succeed, even if as empty
    return hr;
}

HRESULT SaveDatabase(PWINBIO_PIPELINE Pipeline) {
    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        return hr;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the state of the pipeline.
    if (storageContext == NULL ||
        Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        return hr;
    }

    CRFP_DATABASE_FILE_HEADER fileHeader;
    DWORD bytesRead;
    UINT32 i;
    if (SetFilePointer(Pipeline->StorageHandle,
        0,
        NULL,
        FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        hr = WINBIO_E_DATABASE_WRITE_ERROR;
        goto cleanup;
    }

    fileHeader.FileMagic = CRFP_DB_MAGIC;
    fileHeader.RecordsCount = (UINT32)storageContext->Database.size();
    fileHeader.RecordSize = sizeof(CRFP_STORAGE_RECORD);
    fileHeader.TemplateSize = storageContext->TemplateSize;

    DebugLog("Saving database header\n");
    if (!WriteFile(Pipeline->StorageHandle,
        &fileHeader,
        sizeof(fileHeader),
        &bytesRead,
        NULL) || bytesRead != sizeof(fileHeader)) {
        DebugLog("Error saving database header!\n");
        hr = WINBIO_E_DATABASE_WRITE_ERROR;
        goto cleanup;
    }

    for (i = 0; i < fileHeader.RecordsCount; i++) {
        DebugLog("Saving record %d\n", i);
        CRFP_STORAGE_RECORD newRecord = { 0 };
        newRecord = storageContext->Database[i];

        newRecord.TemplateData = NULL;
        newRecord.StorageRecordSize = sizeof(newRecord);

        if (!WriteFile(Pipeline->StorageHandle, &newRecord,
            sizeof(newRecord), &bytesRead, NULL) ||
            bytesRead != sizeof(newRecord)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugLog("Error saving record 0x%x\n", hr);
            goto cleanup;
        }
    }

    for (i = 0; i < fileHeader.RecordsCount; i++) {
        DebugLog("Saving template for record %d\n", i);
        PCRFP_STORAGE_RECORD record = &storageContext->Database[i];

        if (!WriteFile(Pipeline->StorageHandle, record->TemplateData,
            record->TemplateSize,
            &bytesRead, NULL) || bytesRead != record->TemplateSize) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugLog("Error saving template 0x%x\n", hr);
            goto cleanup;
        }
    }

    SetEndOfFile(Pipeline->StorageHandle);

cleanup:
    return hr;
}

BOOLEAN MatchSubject(
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    CRFP_STORAGE_RECORD record
)
{
    // Verify the Identity argument.
    if (Identity->Type != WINBIO_ID_TYPE_GUID &&
        Identity->Type != WINBIO_ID_TYPE_SID &&
        Identity->Type != WINBIO_ID_TYPE_WILDCARD)
    {
        return false;
    }

    if (Identity->Type == WINBIO_ID_TYPE_WILDCARD &&
        Identity->Value.Wildcard != WINBIO_IDENTITY_WILDCARD)
    {
        return false;
    }

    // WINBIO_SUBTYPE_ANY is a valid sub-factor.
    // WINBIO_SUBTYPE_NO_INFORMATION is not a valid sub-factor.
    if (SubFactor == WINBIO_SUBTYPE_NO_INFORMATION)
    {
        return false;
    }

    if (Identity->Type == WINBIO_ID_TYPE_WILDCARD) {
        return true;
    }

    if ((SubFactor == WINBIO_SUBTYPE_ANY || SubFactor == record.SubFactor) &&
        Identity->Type == record.Identity.Type) {
        if (record.Identity.Type == WINBIO_ID_TYPE_GUID &&
            (memcmp(&Identity->Value.TemplateGuid, &record.Identity.Value.TemplateGuid, sizeof(Identity->Value.TemplateGuid)) == 0)) {
            return true;
        }
        else if (record.Identity.Type == WINBIO_ID_TYPE_SID &&
            Identity->Value.AccountSid.Size == record.Identity.Value.AccountSid.Size &&
            Identity->Value.AccountSid.Size <= sizeof(Identity->Value.AccountSid.Data) &&
            (memcmp(Identity->Value.AccountSid.Data, record.Identity.Value.AccountSid.Data, Identity->Value.AccountSid.Size) == 0)) {
            return true;
        }
    }
    return false;
}

HRESULT SyncDatabaseToMCU(PWINBIO_PIPELINE Pipeline) {
    HRESULT hr = S_OK;

    // Verify that pointer arguments are not NULL.
    if (!ARGUMENT_PRESENT(Pipeline))
    {
        hr = E_POINTER;
        return hr;
    }

    // Retrieve the context from the pipeline.
    PWINIBIO_STORAGE_CONTEXT storageContext =
        (PWINIBIO_STORAGE_CONTEXT)Pipeline->StorageContext;

    // Verify the state of the pipeline.
    if (storageContext == NULL ||
        Pipeline->StorageHandle == INVALID_HANDLE_VALUE)
    {
        hr = WINBIO_E_INVALID_DEVICE_STATE;
        return hr;
    }

    //Make sure fpseed is set
    hr = WbioSensorReset(Pipeline);
    if (FAILED(hr)) {
        DebugLog("Calibrate failed\n");
        return hr;
    }

    hr = ResetFPContext(Pipeline);
    if (FAILED(hr)) {
        DebugLog("FP Context reset failed\n");
        return hr;
    }

    for (int i = 0; i < storageContext->Database.size(); i++) {
        CRFP_STORAGE_RECORD record = storageContext->Database[i];
        hr = UploadTemplate(Pipeline, record.TemplateData, record.TemplateSize);
        if (FAILED(hr)) {
            DebugLog("Failed to upload template %d\n", i);
            goto cleanup;
        }
    }

cleanup:
    /*if (FAILED(hr)) {
        DebugLog("EC Console:\n\n=======================================\n");
        PrintConsole(Pipeline);
        DebugLog("=======================================\nEnd Console\n");
    }*/
    if (FAILED(hr)) {
        WINBIO_IDENTITY Identity = { 0 };
        Identity.Type = WINBIO_ID_TYPE_WILDCARD;
        Identity.Value.Wildcard = WINBIO_IDENTITY_WILDCARD;
        WbioStorageDeleteRecord(Pipeline, &Identity, WINBIO_SUBTYPE_ANY);
    }

    return hr;
}