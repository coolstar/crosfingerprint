/*++
Copyright (c) Microsoft Corporation. All Rights Reserved.
Sample code. Dealpoint ID #843729.

Module Name:

uart.h

Abstract:

This module contains the touch driver I2C helper definitions.

Environment:

Kernel Mode

Revision History:

--*/

#pragma once

#include <Windows.h>
#include <wdf.h>

#define RESHUB_USE_HELPER_ROUTINES

//
// UART context
//

typedef struct _UART_CONTEXT
{
	WDFIOTARGET SpbIoTarget;
	LARGE_INTEGER UartResHubId;
	WDFWAITLOCK UartLock;
	WDFREQUEST UartRequest;
} UART_CONTEXT;

VOID
UartTargetDeinitialize(
	IN WDFDEVICE FxDevice,
	IN UART_CONTEXT* SpbContext
);

NTSTATUS
UartTargetInitialize(
	IN WDFDEVICE FxDevice,
	IN UART_CONTEXT* SpbContext
);

NTSTATUS
UartWrite(
	IN UART_CONTEXT* SpbContext,
	PVOID Data,
	ULONG Len
);

NTSTATUS
UartRead(
	IN UART_CONTEXT* SpbContext,
	PVOID Data,
	ULONG Len
);