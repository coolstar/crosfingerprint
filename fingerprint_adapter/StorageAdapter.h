/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    StorageAdapter.h

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
#pragma once

#include "winbio_adapter.h"
#include <vector>

typedef struct _CRFP_STORAGE_RECORD {
    UINT32 StorageRecordSize;

    UINT32 TemplateSize;
    PUCHAR TemplateData;

    WINBIO_BIOMETRIC_SUBTYPE SubFactor;
    WINBIO_IDENTITY Identity;
} CRFP_STORAGE_RECORD, *PCRFP_STORAGE_RECORD;

#define CRFP_DB_MAGIC 'CRDB'

typedef struct _CRFP_DATABASE_FILE_HEADER {
    UINT32 FileMagic;
    UINT32 RecordsCount;
    UINT32 RecordSize;
    UINT32 TemplateSize;

    GUID DataFormat;
    UINT32 IndexElementCount;

    //CRFP_STORAGE_RECORD Records[];
    //PUCHAR Templates[];
} CRFP_DATABASE_FILE_HEADER, *PCRFP_DATABASE_FILE_HEADER;

///////////////////////////////////////////////////////////////////////////////
//
// The WINIBIO_STORAGE_CONTEXT structure is privately-defined by each
// Storage Adapter. Its purpose is to maintain any information that
// should persist across Storage Adapter API calls. 
//
// The Adapter allocates and initializes one of these structures in its
// 'Attach' routine and saves its address in the Pipeline->StorageContext 
// field.
//
// The Storage Adapter's 'Detach' routine cleans up and deallocates the
// structure and sets the PipelineContext->StorageContext field to NULL.
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _WINIBIO_STORAGE_CONTEXT {
    UINT16 FingerWidth;
    UINT16 FingerHeight;
    UINT32 MaxFingers;
    UINT32 TemplateSize;

    GUID DataFormat;
    UINT32 IndexElementCount;

    BOOLEAN AllowCreateOnOpen;

    ULONG RecordCursor;
    std::vector<CRFP_STORAGE_RECORD> Database;
    std::vector<PCRFP_STORAGE_RECORD> Records;

} WINIBIO_STORAGE_CONTEXT, *PWINIBIO_STORAGE_CONTEXT;

HRESULT LoadDatabase(PWINBIO_PIPELINE Pipeline);
HRESULT SaveDatabase(PWINBIO_PIPELINE Pipeline);
HRESULT SyncDatabaseToMCU(PWINBIO_PIPELINE Pipeline);

BOOLEAN MatchSubject(
    _In_ PWINBIO_IDENTITY Identity,
    _In_ WINBIO_BIOMETRIC_SUBTYPE SubFactor,
    CRFP_STORAGE_RECORD record
);