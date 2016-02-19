/**
 * Wow64 Wrapper
 * Last modified: 2008/12/13
 **/

#include "stdafx.h"
#include "Wow64.h"

///////////////////////////////////////////////////////////////////////////////

#ifndef _WIN64

#define GetProc(m, n)       ((void*) GetProcAddress( GetModuleHandleA(m), n))

typedef BOOL (WINAPI *PFNISWOW64PROCESS)          ( HANDLE hProcess, PBOOL Wow64Process );
typedef LONG (WINAPI *PFNREGDISABLEREFLECTIONKEY) ( HKEY hBase );

///////////////////////////////////////////////////////////////////////////////

BOOL WINAPI Wow64CheckProcess()
{
       PFNISWOW64PROCESS
       pfnIsWow64Process =
      (PFNISWOW64PROCESS) GetProc( "KERNEL32.dll",
         "IsWow64Process");                BOOL bIsWow64;
   if (pfnIsWow64Process &&
       pfnIsWow64Process( GetCurrentProcess(), &bIsWow64))
      return                                   (bIsWow64);
   else return(FALSE);
}

///////////////////////////////////////////////////////////////////////////////

void WINAPI Wow64DisableRegReflection( HKEY hKey )
{
       PFNREGDISABLEREFLECTIONKEY
       pfnRegDisableReflectionKey =
      (PFNREGDISABLEREFLECTIONKEY) GetProc( "ADVAPI32.dll",
         "RegDisableReflectionKey");
   if (pfnRegDisableReflectionKey && hKey)
      pfnRegDisableReflectionKey    (hKey);
}

#endif

///////////////////////////////////////////////////////////////////////////////
