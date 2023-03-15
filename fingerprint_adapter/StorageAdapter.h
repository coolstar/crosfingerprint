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
    WINBIO_IDENTITY Identity;
    WINBIO_BIOMETRIC_SUBTYPE SubFactor;
} CRFP_STORAGE_RECORD, *PCRFP_STORAGE_RECORD;

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
    //
    // The following fields illustrate the kind of information 
    // the Storage Adapter needs to keep in this structure:
    //
    //      DatabaseId      - Identify of the Adapter's 
    //                        currently-opened database.
    //
    //      DatabaseHandle  - A handle to the Storage Adapter's 
    //                        currently-open database.
    //
    //      ResultSet       - A collection of records generated by
    //                        a database query operation. 
    //
    //      ResultSetCursor - Current location in the result 
    //                        set. Used to iterate through the
    //                        result set and make individual
    //                        records available.
    //
    WINBIO_UUID DatabaseId;
    PVOID DatabaseHandle;

    UINT32 MaxFingers;
    UINT32 TemplateSize;

    size_t DatabaseCursor;
    std::vector<CRFP_STORAGE_RECORD> Database;

} WINIBIO_STORAGE_CONTEXT, *PWINIBIO_STORAGE_CONTEXT;

HRESULT ec_command(PWINBIO_PIPELINE Pipeline, int cmd, int version, const void* outdata, int outsize, void* indata, int insize);

void DebugLog(const char* format, ...);