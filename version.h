/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

#include "targetver.h"      // Minimum supported Windows platform
#include "debug64.h"        // Normalize build architecture #defines
                            // (i.e. _WIN32, _WIN64, _32_BIT, _64_BIT)

// ----------------------------------------------------------------------------
// Architecture name strings

#if defined(_DEBUG)
  #if defined(_64_BIT)
    #define ARCH_NAME_A             "64-bit DEBUG"
  #else
    #define ARCH_NAME_A             "32-bit DEBUG"
  #endif
#else // Release
  #if defined(_64_BIT)
    #define ARCH_NAME_A             "64-bit"
  #else
    #define ARCH_NAME_A             "32-bit"
  #endif
#endif

#define ARCH_NAME                   _T(ARCH_NAME_A)

// ----------------------------------------------------------------------------
// PE version MUST be in the form:      major.minor

#ifdef _USRDLL
  #pragma comment( linker,    "/version:3.0" )
#endif

// ----------------------------------------------------------------------------
// String version may be in any format

#undef       USE_ABC                // (undefine before #defining)
#define      USE_ABC                // (use AutoBuildCount header)
#if !defined(USE_ABC)
  #define VERSION_NUM                    3,0,0,0
  #define VERSION_STR_A                 "3.0.0.0"
  #define VERSION_STR               _T( "3.0.0.0" )
#else
  #include "AutoBuildCount.h"       // (ABC is where SVNREV is #defined)
  #define VERSION_NUM                    3,0,0,         SVNREV_NUM
  #define VERSION_STR_A                 "3.0.0."        SVNREV_STR
  #define VERSION_STR               _T( "3.0.0." )  _T( SVNREV_STR )
#endif

// ----------------------------------------------------------------------------
