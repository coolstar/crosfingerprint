/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    precomp.h

Abstract:

    This module contains identifies all the headers that
    shoulde be pre-compiled.

Author:

    -

Environment:

    Win32, user mode only.

Revision History:

NOTES:

    (None)

--*/
#pragma once

//
// Necessary for compiling under VC.
//
#if(!defined(WINVER) || (WINVER < 0x0500))
	#undef WINVER
	#define WINVER          0x0500
#endif
#if(!defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500))
	#undef _WIN32_WINNT
	#define _WIN32_WINNT    0x0500
#endif

//
// Required header files that shouldn't change often.
//
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

//
// StrSafe.h needs to be included last
// to disallow unsafe string functions.
#include <STRSAFE.H>

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(x) ((x) != NULL)
#endif

void DebugLog(const char* format, ...);

#include "winbio_adapter.h"
HRESULT ec_command(PWINBIO_PIPELINE Pipeline, int cmd, int version, const void* outdata, int outsize, void* indata, int insize);
HRESULT DownloadTemplate(PWINBIO_PIPELINE Pipeline, PUCHAR* outBuffer, UINT32 templateSize, int idx);