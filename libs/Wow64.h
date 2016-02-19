/**
 * Wow64 Wrapper
 * Last modified: 2008/12/13
 **/

#pragma once

///////////////////////////////////////////////////////////////////////////////

#if defined( _32_BIT )

BOOL WINAPI Wow64CheckProcess();
void WINAPI Wow64DisableRegReflection( HKEY hKey );

///////////////////////////////////////////////////////////////////////////////
#else // _64_BIT

#define Wow64CheckProcess()         FALSE
#define Wow64DisableRegReflection   RegDisableReflectionKey

#endif // _32_BIT || _64_BIT

///////////////////////////////////////////////////////////////////////////////
