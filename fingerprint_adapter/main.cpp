#include "precomp.h"

///////////////////////////////////////////////////////////////////////////////
//
// Mandatory DLL entrypoint function.
//
///////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
DllMain(
    HANDLE ModuleHandle,
    DWORD ReasonForCall,
    LPVOID Reserved
)
{
    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(ReasonForCall);
    UNREFERENCED_PARAMETER(Reserved);

    return TRUE;
}
//-----------------------------------------------------------------------------