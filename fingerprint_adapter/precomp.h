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

#if 0
#include <time.h>

void DebugLog_internal(const char* format, ...);

#define DebugLog(fmt, ...) \
do {\
time_t mytime = time(NULL); \
char time_str[26];\
ctime_s(time_str, sizeof(time_str), &mytime);\
time_str[strlen(time_str) - 1] = '\0'; \
DebugLog_internal("[%s] " fmt, time_str, __VA_ARGS__);\
} while(0)
#else
#define DebugLog(x, ...)
#endif

#include "winbio_adapter.h"
#pragma warning(disable:4200)  // suppress nameless struct/union warning
#include "../crosfingerprint/ec_commands.h"
#pragma warning(default:4200)

HRESULT ec_command(PWINBIO_PIPELINE Pipeline, int cmd, int version, const void* outdata, int outsize, void* indata, int insize);
//HRESULT PrintConsole(PWINBIO_PIPELINE Pipeline);

HRESULT DownloadTemplate(PWINBIO_PIPELINE Pipeline, PUCHAR* outBuffer, UINT32 templateSize, int idx);

HRESULT ResetFPContext(PWINBIO_PIPELINE Pipeline);

HRESULT UploadTemplate(PWINBIO_PIPELINE Pipeline, PUCHAR buffer, UINT32 templateSize);

enum StorageControlCode {
    StorageControlCodeNone,
    StorageControlCodeUploadToHw,
    StorageControlCodeSaveToDisk,
    StorageControlDownloadTemplate
};