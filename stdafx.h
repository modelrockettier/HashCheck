//
// Windows Hashing/Checksumming Library
// Last modified: 2009/01/13
// Copyright (C) Kai Liu.  All rights reserved.
//
// "C++" include file for standard system include files, or
// project specific include files that are used frequently,
// but are otherwise changed relatively infrequently.
//

#pragma once

//////////////////////////////////////////////////////////////////////////////////////////
//
//                    **  IMPORTANT PROGRAMMING NOTE  **
//
//////////////////////////////////////////////////////////////////////////////////////////
//
//  The following "Vista" settings are unfortunately *REQUIRED* for HashCheck
//  due to its use of the RegDeleteTree function which only comes with Vista. 
//
//////////////////////////////////////////////////////////////////////////////////////////
// We need to #define these before #including "TargetVer.h".  See above for why!

#define  _WIN32_WINNT     0x0600      // Windows Vista
#define  WINVER           0x0600      // Windows Vista
#define  NTDDI_VERSION    0x06000100  // Windows Vista with SP1
#define  _WIN32_IE        0x0800      // Internet Explorer 8.0

#include "TargetVer.h"            // (SoftDevLabs standard)

///////////////////////////////////////////////////////////////////////////////
// Compiler warning/error overrides...

#pragma warning( disable: 4200 )  // "nonstandard extension used : zero-sized array in struct/union"
#pragma warning( error:   4702 )  // "unreachable code" is a SERIOUS ERROR!

///////////////////////////////////////////////////////////////////////////////
// (helper macros to "stringize" a macro value)

#define QSTR2(x)    #x
#define QSTR(x)     QSTR2(x)
#define QWSTR2(x)   L ## x
#define QWSTR(x)    QWSTR2(x)

///////////////////////////////////////////////////////////////////////////////
// Check for minimum supported platform...

#include "targetver.h"              // Minimum supported Windows platform

#if VS_VERSION < MIN_VS_VERSION
  #pragma message(__FILE__"("QSTR(__LINE__)") : error MIN_VS_VERSION : Visual Studio compiler version "QSTR(MIN_VS_VERSION)" or greater is required. (Sorry!)")
  #error (see previous message)
#endif

#ifdef _M_IA64
  #error Itanium builds are no longer supported.  (Sorry!)
#endif

///////////////////////////////////////////////////////////////////////////////
// Check for required build settings:  the properties for our Visual Studio
// project should have the value of 'TARGETNAME' being passed to us in its
// Preprocessor Definitions settings (e.g. TARGETNAME=\"$(TargetName)\")

#if !defined( TARGETNAME )
  #error TARGETNAME undefined. Check your project''s Preprocessor Definitions.
#endif

///////////////////////////////////////////////////////////////////////////////
// Normalize build architecture #defines (_WIN32, _WIN64, _32_BIT, _64_BIT)

#include "debug64.h"

///////////////////////////////////////////////////////////////////////////////
// System headers...

#include <windows.h>
#include <intrin.h>
#include <memory.h>
#include <olectl.h>
#include <process.h>
#include <propidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <uxtheme.h>
#include <atlstr.h>
#include <atlpath.h>
#include <atlcoll.h>
#include <comdef.h>

///////////////////////////////////////////////////////////////////////////////
// Use new WinXP style skin/buttons/etc...

#pragma comment                                                             \
(                                                                           \
    linker,                                                                 \
    "\""                                                                    \
        "/manifestdependency:"                                              \
            "type                  = 'Win32'                             "  \
            "name                  = 'Microsoft.Windows.Common-Controls' "  \
            "version               = '6.0.0.0'                           "  \
            "processorArchitecture = '*'                                 "  \
            "publicKeyToken        = '6595b64144ccf1df'                  "  \
    "\""                                                                    \
)

///////////////////////////////////////////////////////////////////////////////
// Required link libraries

#pragma comment( lib, "shlwapi"  )
#pragma comment( lib, "comctl32" )
#pragma comment( lib, "uxtheme"  )

///////////////////////////////////////////////////////////////////////////////
// Macros, constants, typedefs, etc. needed by ALL project modules.
// Common project-wide headers are #included afterwards at the end.

// Cache alignment macros for efficiency

#if defined(_M_X64)
  #define CACHE_LINE_SIZE           64
#else //  (_M_IX86)
  #define CACHE_LINE_SIZE           32
#endif
#define CACHE_ALIGN                 __declspec( align( CACHE_LINE_SIZE ))

// Unaligned types

typedef UNALIGNED PWORD  UPWORD;
typedef UNALIGNED PDWORD UPDWORD;

// Volatile types

typedef volatile long VLONG, *PVLONG;
typedef volatile UINT MSGCOUNT, *PMSGCOUNT;

// Threading typedefs

typedef unsigned (__stdcall    BEGINTHREADEXPROC)(void*);
typedef unsigned (__stdcall  *PBEGINTHREADEXPROC)(void*);
typedef void     (__fastcall *PFNWORKERMAIN)     (void*);

// Helper macros

#define CRLF                        "\r\n"

#define BYTEDIFF(a, b)              ((PBYTE)(a) - (PBYTE)(b))
#define BYTEADD(a, cb)              ((void*)((PBYTE)(a) + (cb)))

#define GETWORD(p)                  (*((UPWORD) (p)))
#define GETDWORD(p)                 (*((UPDWORD)(p)))

#define PUTDWORD(p,d)               (*((UPDWORD)(p)) = (d))

///////////////////////////////////////////////////////////////////////////////
// Define globals for DLL bookkeeping, Activation and Windows Version

extern HMODULE                      g_hModThisDll;
extern volatile BOOL                g_bActCtxCreated;
extern HANDLE                       g_hActCtx;
extern UINT16                       g_wWinVer;

///////////////////////////////////////////////////////////////////////////////
// User-interface tuning constants

#define HC_FONT_PREF_NAME           _T("Consolas")
#define HC_FONT_DEF_NAME            _T("Lucida Console")
#define HC_FONT_POINT_SIZE          9

#define KB                          (1024)
#define MB                          (KB * KB)
#define MAX_PATH_BUFFER             (KB * 2)
#define READ_BUFFER_SIZE            (KB * 256)

#define MAX_INFLIGHT                128     // maximum simultaneously active I/Os
#define MARQUEE_INTERVAL            100     // milliseconds between UI updates
#define MAX_STRINGRES               128     // max translation string length; increase as needed
#define MAX_STRINGMSG               256     // max translation string length; increase as needed

// Progress bar states

#ifndef PBM_SETSTATE
#define PBM_SETSTATE                (WM_USER + 16)
#define PBST_NORMAL                 0x0001
#define PBST_PAUSED                 0x0003
#endif

// Codes

#define TIMER_ID_PAUSE              1

// Flags

#define HCF_EXIT_PENDING            0x0001
#define HCF_MARQUEE                 0x0002
#define HVF_HAS_SET_TYPE            0x0004
#define HVF_ITEM_HILITE             0x0008
#define HPF_HAS_RESIZED             0x0004

///////////////////////////////////////////////////////////////////////////////
// These messages are sent as each file in the list is done being processed.
// They are sent as each worker thread finishes. First the update message is
// sent, then the done message. The update message causes the UI to display
// the hash results, and the done message causes the success/failure totals
// and number of remaining files totals tp be updated. Worker threads are
// created serially as each entry in the list is processed, and the next one
// is not created/started until the previous one is finished.

#define HM_WORKERTHREAD_DONE        (WM_APP + 0)  // wParam = ctx, lParam = 0
#define HM_WORKERTHREAD_UPDATE      (WM_APP + 1)  // wParam = ctx, lParam = data

//-----------------------------------------------------------------------------
// This message is sent by the worker thread to tell the UI the size in bytes
// of the file being processed, since it is the worker thread which opens and
// processes each file. It is only sent if the last parameter passed to the
// "WorkerThreadHashFile" function (which points to a FILESIZE struct) is not
// NULL. Only HashVerify cares about the file size since the file size is one
// of the columns in its list view. HashProp/HashSave both pass NULL for this
// parameter since they don't care about the file size.

#define HM_WORKERTHREAD_SETSIZE     (WM_APP + 2)  // wParam = ctx, lParam = filesize

//-----------------------------------------------------------------------------
// This message is sent immediately before/after "preparing" the list of files
// which involves expanding each entry to their full path, etc. The call made
// beforehand causes the progress bar to get set into marquee mode to indicate
// progress is indeterminate, and the afterwards call sets is back to normal
// mode where it displays an actual progress percentage for each file as they
// are processed. This preparation step could conceivably take a long time if
// we're asked to hash thousands of files in hundreds of subdirectories.
// 
#define HM_WORKERTHREAD_TOGGLEPREP  (WM_APP + 3)  // wParam = ctx, lParam = state

//-----------------------------------------------------------------------------
// The PBM_SETPOS message is sent to our progress control every UI update
// interval (i.e. MARQUEE_INTERVAL, currently set to 100 milliseconds) as
// our worker thread processes a given file. The position value it sends
// is the percentage of the file that has been processed so far, expressed
// in fractions of a percent (i.e. from 0 to 100 * HM_PERCENT_FRACTION).
// Set HM_PERCENT_FRACTION to 1 (one) for course whole percentage updates
// or a larger value for smaller more finely grained percentage updates.
//
// PROGRAMMING NOTE: please also note that this value also limits the max-
// imum file size that we can accurately report progress for (due to the
// integer arithmetic involved; see the WorkerThreadHashFile function).
//
// The current value of 10 (tenths of a percent) limits the size of a file
// that we can accurately report a percentage progress for to 12 petabytes
// which should be plenty good enough for now. If we ever need to support
// file sizes larger than this, we always have the option of changing the
// calculation logc in the WorkerThreadHashFile function to use floating-
// point arithmetic instead of integer arithmetic and then converting the
// result back to value from 0 to HM_PERCENT_FACTOR for our progress bar.

#define HM_PERCENT_FRACTION         10            // tenths of a percent (0.1%)
#define HM_PERCENT_FACTOR          (100 * HM_PERCENT_FRACTION)

///////////////////////////////////////////////////////////////////////////////
// Project-wide global variables...

extern UINT16            g_wWinVer;
extern HMODULE           g_hModThisDll;
extern volatile BOOL     g_bActCtxCreated;
extern HANDLE            g_hActCtx;

///////////////////////////////////////////////////////////////////////////////
// Project-wide helper functions...

extern CString  HResultToString( HRESULT hr );
extern CString  HResultMessage ( HRESULT hr );
extern CString  FormatLastError( DWORD dwLastError = GetLastError() );

extern BOOL     InitializeDll( HINSTANCE hInstance );

extern LONG     IncrementDllReference();
extern LONG     DecrementDllReference();
extern LONG     CurrentDllReference();
extern VLONG*   PointerToDllReference();

extern CPath    FullPathSysDir( bool bEndWithBackSlash = false );
extern CPath    FullPathDll();

extern CPath    TempPath( bool bEndWithBackSlash = false );
extern CPath    TempName( LPCTSTR pszDirectory = NULL );

///////////////////////////////////////////////////////////////////////////////
// Project-wide common headers...

#include "product.h"
#include "version.h"
#include "dbgtrace.h"
#include "RegHelpers.h"
#include "HashCheckResources.h"
#include "HashCheckTranslations.h"

#include "libs\SwapIntrinsics.h"
#include "libs\SimpleString.h"
#include "libs\SimpleList.h"
#include "libs\Wow64.h"

#include "HashCheckCommon.h"
#include "CHashCheckClassFactory.h"
#include "CHashCheck.h"

///////////////////////////////////////////////////////////////////////////////
