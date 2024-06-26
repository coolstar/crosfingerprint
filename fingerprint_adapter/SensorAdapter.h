/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    SensorAdapter.h

Abstract:

    This module contains a stub implementation of an Sensor Adapter
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
#include "../crosfingerprint/crosfpdata.h"
#include <winbio_ioctl.h>

///////////////////////////////////////////////////////////////////////////////
//
// The WINIBIO_SENSOR_CONTEXT structure is privately-defined by each
// Sensor Adapter. Its purpose is to maintain any information that
// should persist across Sensor Adapter API calls. 
//
// The Adapter allocates and initializes one of these structures in its
// 'Attach' routine and saves its address in the Pipeline->SensorContext 
// field.
//
// The Sensor Adapter's 'Detach' routine cleans up and deallocates the
// structure and sets the PipelineContext->SensorContext field to NULL.
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _WINIBIO_SENSOR_CONTEXT {
    OVERLAPPED Overlapped;
    CRFP_CAPTURE_DATA CaptureData;
} WINIBIO_SENSOR_CONTEXT, *PWINIBIO_SENSOR_CONTEXT;