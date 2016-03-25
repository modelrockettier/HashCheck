/**
 * HashCheck Shell Extension
 * Original work copyright (C) Kai Liu.  All rights reserved.
 * Modified work copyright (C) 2014 Christopher Gurnee.  All rights reserved.
 * Modified work copyright (C) 2016 Tim Schlueter.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "version_generated.h"

// Full name; this will appear in the version info and control panel
#define HASHCHECK_NAME_STR "HashCheck Shell Extension"

#if 0
#define HASHCHECK_VERSION_MAJOR 3
#define HASHCHECK_VERSION_MINOR 0
#define HASHCHECK_VERSION_REVISION 1
#define HASHCHECK_VERSION_BUILD 1

// Uninstaller Info
#define HASHCHECK_ABOUT_URL "https://github.com/modelrockettier/HashCheck/"
#define HASHCHECK_HELP_URL "https://github.com/modelrockettier/HashCheck/releases"
#define HASHCHECK_PUBLISHER "Tim Schlueter"
#endif

// String used in the "CompanyName" field of the version resource
//#define HASHCHECK_AUTHOR_STR HASHCHECK_ABOUT_URL

// Tail portion of the copyright string for the version resource
//#define HASHCHECK_COPYRIGHT_STR "Kai Liu, Christopher Gurnee, David B. Trout, Tim Schlueter. All rights reserved."

// Name of the DLL
#define HASHCHECK_FILENAME_STR "HashCheck.dll"

// Full version: MUST be in the form of major,minor,revision,build
//#define HASHCHECK_VERSION_FULL HASHCHECK_VERSION_MAJOR,HASHCHECK_VERSION_MINOR,HASHCHECK_VERSION_REVISION,HASHCHECK_VERSION_BUILD

// String version: May be any suitable string
//#define HASHCHECK_VERSION_STR (#HASHCHECK_VERSION_MAJOR "." #HASHCHECK_VERSION_MINOR "." #HASHCHECK_VERSION_REVISION "." #HASHCHECK_VERSION_BUILD)

#ifdef _USRDLL
// PE version: MUST be in the form of major.minor
//#pragma comment(linker, "/version:3.0")
#endif

// String used for setup
#define HASHCHECK_SETUP_STR "HashCheck Shell Extension Setup"

// Architecture names
#if defined(_M_IX86)
#define ARCH_NAME_TAIL " (x86-32)"
#define ARCH_NAME_FULL "x86-32"
#define ARCH_NAME_PART "x86"
#elif defined(_M_AMD64) || defined(_M_X64)
#define ARCH_NAME_TAIL " (x86-64)"
#define ARCH_NAME_FULL "x86-64"
#define ARCH_NAME_PART "x64"
#elif defined(_M_IA64)
#define ARCH_NAME_TAIL " (IA-64)"
#define ARCH_NAME_FULL "IA-64"
#define ARCH_NAME_PART ARCH_NAME_FULL
#else
#define ARCH_NAME_TAIL ""
#define ARCH_NAME_FULL "Unspecified"
#define ARCH_NAME_PART ARCH_NAME_FULL
#endif
