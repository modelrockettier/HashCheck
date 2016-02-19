//
// Windows Hashing/Checksumming Library
// Last modified: 2009/01/13
// Copyright (C) Kai Liu.  All rights reserved.
//
// "C++" source file that includes just the standard includes
// stdafx.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information
//

#include "stdafx.h"         // pre-compiled headers for "C++" programs

///////////////////////////////////////////////////////////////////////////////
// Our DLL module's global variables...

static CPath      g_strFullPathSysDir;
static CPath      g_strFullPathDll;
static CPath      g_strFullTempPath;
static VLONG      g_cRefThisDll       = 0;
       UINT16     g_wWinVer           = 0;
       HMODULE    g_hModThisDll       = NULL;
volatile BOOL     g_bActCtxCreated    = FALSE;
       HANDLE     g_hActCtx           = NULL;

///////////////////////////////////////////////////////////////////////////////
// Initialize global variables...   (called by DllMain DLL_PROCESS_ATTACH)

BOOL InitializeDll( HINSTANCE hInstance )
{
    OSVERSIONINFO osvi = {sizeof( OSVERSIONINFO ), 0};

    VERIFY( GetVersionEx( &osvi ));

    g_wWinVer = MAKEWORD( (BYTE) osvi.dwMinorVersion,
                          (BYTE) osvi.dwMajorVersion );

    // Check if they are trying to install us on a
    // Windows Platform we're not designed to support

    if (g_wWinVer < WINVER)
    {
        HCTRACE(_T("ERROR: minimum supported WINVER is 0x%04.4X\n"), WINVER );
        return FALSE;
    }

    g_hModThisDll    = hInstance;
    g_hActCtx        = INVALID_HANDLE_VALUE;
    g_bActCtxCreated = FALSE;

    *PointerToDllReference() = 0;

    VERIFY( FullPathDll() );
    VERIFY( FullPathSysDir() );

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Return a pointer to our reference counter

VLONG* PointerToDllReference()
{
    return &g_cRefThisDll;
}

///////////////////////////////////////////////////////////////////////////////
// Atomically increment our reference count

LONG IncrementDllReference()
{
    LONG cRef = InterlockedIncrement( PointerToDllReference() );
    HCTRACE(_T("++ref now = %d\n"), cRef );
    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
// Atomically decrement our reference count

LONG DecrementDllReference()
{
    LONG cRef = InterlockedDecrement( PointerToDllReference() );
    HCTRACE(_T("--ref now = %d\n"), cRef );
    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
// Return our current reference count

LONG CurrentDllReference()
{
    return *PointerToDllReference();
}

///////////////////////////////////////////////////////////////////////////////
// (Helper functions)

static CPath AddTrailingBackSlash( CPath& strPath )
{
    strPath.AddBackslash();
    return strPath;
}

static CPath RemoveTrailingBackSlash( CPath& strPath )
{
    strPath.RemoveBackslash();
    return strPath;
}

///////////////////////////////////////////////////////////////////////////////
// CString GetSystemDirectory       (i.e. %SystemRoot%\System32)

CPath FullPathSysDir( bool bEndWithBackSlash /* =false */)
{
    CString strFullPathSysDir = g_strFullPathSysDir;

    if (strFullPathSysDir.IsEmpty())
    {
        ASSERT( g_hModThisDll );

        if (!g_hModThisDll)
            return CPath(_T(""));

        DWORD dwRetTChars, dwBuffTChars = 0;
        LPTSTR pszFullPath;

        do
        {
            dwBuffTChars += MAX_PATH;

            if (!(pszFullPath = strFullPathSysDir.GetBuffer( dwBuffTChars + 1 )))
                break;

            dwRetTChars = GetSystemDirectory( pszFullPath, dwBuffTChars );
        }
        while (dwRetTChars >= dwBuffTChars);

        if (!pszFullPath)
        {
            APIFAILEDMSG( strFullPathSysDir.GetBuffer );
            return CPath(_T(""));
        }

        if (!dwRetTChars)
        {
            APIFAILEDMSG( GetSystemDirectory );
            return CPath(_T(""));
        }

        strFullPathSysDir.ReleaseBuffer( dwRetTChars );
        g_strFullPathSysDir = CPath( strFullPathSysDir );

        HCTRACE(_T("\"%s\"\n"), g_strFullPathSysDir );
    }

    if (bEndWithBackSlash)
        return AddTrailingBackSlash( g_strFullPathSysDir );
    else
        return RemoveTrailingBackSlash( g_strFullPathSysDir );
}

///////////////////////////////////////////////////////////////////////////////
// CString GetModuleFilename

CPath  FullPathDll()
{
    CString strFullPathDll = g_strFullPathDll;

    if (strFullPathDll.IsEmpty())
    {
        ASSERT( g_hModThisDll );

        if (!g_hModThisDll)
            return CPath(_T(""));

        DWORD dwRetTChars, dwBuffTChars = 0;
        LPTSTR pszFullPath;

        do
        {
            dwBuffTChars += MAX_PATH;

            if (!(pszFullPath = strFullPathDll.GetBuffer( dwBuffTChars + 1 )))
                break;

            dwRetTChars = GetModuleFileName( g_hModThisDll, pszFullPath, dwBuffTChars );
        }
        while (dwRetTChars >= dwBuffTChars);

        if (!pszFullPath)
        {
            APIFAILEDMSG( strFullPathDll.GetBuffer );
            return CPath(_T(""));
        }

        if (!dwRetTChars)
        {
            APIFAILEDMSG( GetModuleFileName );
            return CPath(_T(""));
        }

        strFullPathDll.ReleaseBuffer( dwRetTChars );
        g_strFullPathDll = CPath( strFullPathDll );

        HCTRACE(_T("\"%s\"\n"), g_strFullPathDll );
    }

    return g_strFullPathDll;
}

///////////////////////////////////////////////////////////////////////////////
// Retrieve the path of the directory designated for temporary files

CPath TempPath( bool bEndWithBackSlash /* =false */)
{
    CString strFullTempPath = g_strFullTempPath;

    if (strFullTempPath.IsEmpty())
    {
        DWORD dwRetTChars, dwBuffTChars = 0;
        LPTSTR pszFullPath;

        do
        {
            dwBuffTChars += MAX_PATH;

            if (!(pszFullPath = strFullTempPath.GetBuffer( dwBuffTChars + 1 )))
                break;

            dwRetTChars = GetTempPath( dwBuffTChars, pszFullPath );
        }
        while (dwRetTChars >= dwBuffTChars);

        if (!pszFullPath)
        {
            APIFAILEDMSG( strFullTempPath.GetBuffer );
            return CPath(_T(""));
        }

        if (!dwRetTChars)
        {
            APIFAILEDMSG( GetTempPath );
            return CPath(_T(""));
        }

        strFullTempPath.ReleaseBuffer( dwRetTChars );
        g_strFullTempPath = CPath( strFullTempPath );

        HCTRACE(_T("\"%s\"\n"), g_strFullTempPath );
    }

    if (bEndWithBackSlash)
        return AddTrailingBackSlash( g_strFullTempPath );
    else
        return RemoveTrailingBackSlash( g_strFullTempPath );
}

///////////////////////////////////////////////////////////////////////////////
// Create a name for a temporary file.

CPath TempName( LPCTSTR pszDirectory /* =NULL */)
{
    CString  strTempName;
    LPTSTR   pszTempName  = strTempName.GetBuffer( MAX_PATH + 1 );

    if (!pszTempName)
    {
        APIFAILEDMSG( strTempName.GetBuffer );
        return CPath(_T(""));
    }

    CString  strDirectory  = pszDirectory ? pszDirectory : TempPath();

    UINT rc = GetTempFileName
    (
        strDirectory,
        _T("HC"),
        0,  // "If uUnique is zero, the function attempts to form a unique
            //  file name using the current system time. If the file already
            //  exists, the number is increased by one and the functions tests
            //  if this file already exists. This continues until a unique
            //  filename is found; the function creates a file by that name
            //  and closes it."
        pszTempName
    );

    // "If the function succeeds, the return value specifies the unique
    //  numeric value used in the temporary file name. If the uUnique
    //  parameter is nonzero, the return value specifies that same number.
    //  If the function fails, the return value is zero."

    if (!rc)
    {
        APIFAILEDMSG( GetTempFileName );
        return CPath(_T(""));
    }

    strTempName.ReleaseBuffer();

    HCTRACE(_T("\"%s\"\n"), strTempName );

    return CPath( strTempName );
}

///////////////////////////////////////////////////////////////////////////////

CString  HResultToString( HRESULT hr )
{
#define RETURN_IF_HR_IS( NAME )     if (NAME == hr) return CString(_T(#NAME))

    RETURN_IF_HR_IS(  S_OK                      );
    RETURN_IF_HR_IS(  S_FALSE                   );
    RETURN_IF_HR_IS(  E_NOTIMPL                 );
    RETURN_IF_HR_IS(  E_FAIL                    );
    RETURN_IF_HR_IS(  E_OUTOFMEMORY             );
    RETURN_IF_HR_IS(  E_UNEXPECTED              );
    RETURN_IF_HR_IS(  SELFREG_E_CLASS           );
    RETURN_IF_HR_IS(  CLASS_E_CLASSNOTAVAILABLE );

    CString str; str.Format(_T("0x%08.8X"), hr); return str;
}

///////////////////////////////////////////////////////////////////////////////

CString  HResultMessage( HRESULT hr )
{
    _com_error com_err( hr ); CString msg;
    msg.Format(_T("%s: %s"), HResultToString( hr ), com_err.ErrorMessage());
    return msg;
}

///////////////////////////////////////////////////////////////////////////////

CString  FormatLastError( DWORD dwLastError /* =GetLastError()*/ )
{
    CString  strLastError;
    LPTSTR   pszMsgBuf;

    pszMsgBuf  = strLastError.GetBuffer( MAX_STRINGMSG + 1 );
    *pszMsgBuf = 0;

    DWORD  dwChars  = ::FormatMessage
    (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwLastError,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        pszMsgBuf,
        MAX_STRINGMSG,
        NULL
    );

    ASSERT( dwChars <= MAX_STRINGMSG );
    *(pszMsgBuf + MAX_STRINGMSG - 1) = 0;
    strLastError.ReleaseBuffer( dwChars );
    strLastError.Remove( _T('\r') );
    strLastError.Remove( _T('\n') );
    return strLastError;
}

///////////////////////////////////////////////////////////////////////////////
