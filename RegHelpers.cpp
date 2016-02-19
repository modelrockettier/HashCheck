/**
 * Registry Helper Functions
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
// Create or open a registry key...
//
// If  pszSubKeyArg not NULL, then pszSubKey is a printf format pattern
// and pszSubKeyArg is the printf argument

HKEY HCRegCreateOpen( HKEY hBaseKey, LPCTSTR pszSubKey, LPCTSTR pszSubKeyArg /* =NULL */)
{
    ASSERT( hBaseKey && pszSubKey );

    CString strSubKey;

    if (pszSubKeyArg)
        strSubKey.Format( pszSubKey, pszSubKeyArg );
    else
        strSubKey = pszSubKey;

    HKEY hKeyRet = NULL;

    long rc = RegCreateKeyEx( hBaseKey, strSubKey, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKeyRet, NULL );

    ASSERT((ERROR_SUCCESS == rc && hKeyRet) || !hKeyRet );
    return (ERROR_SUCCESS == rc) ? hKeyRet   :   NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Delete a specific registry value...
//
// If  pszValueNameArg not NULL, then pszValueName is a printf format pattern
// and pszValueNameArg is the printf argument

bool HCRegDeleteValue( HKEY hBaseKey, LPCTSTR pszValueName, LPCTSTR pszValueNameArg /* =NULL */)
{
    ASSERT( hBaseKey && pszValueName );

    CString strValueName;

    if (pszValueNameArg)
        strValueName.Format( pszValueName, pszValueNameArg );
    else
        strValueName = pszValueName;

    // Delete the specific registry value

    long rc = RegDeleteValue( hBaseKey, strValueName );

    return (ERROR_SUCCESS == rc || ERROR_FILE_NOT_FOUND == rc);
}

///////////////////////////////////////////////////////////////////////////////
// Delete an entire registry tree...
//
// If  pszTreeArg not NULL, then pszTreeSubKey is a printf format pattern
// and pszTreeArg is the printf argument

bool HCRegDeleteTree( HKEY hTreeBase, LPCTSTR pszTreeSubKey, LPCTSTR pszTreeArg /* =NULL */)
{
    ASSERT( hTreeBase && pszTreeSubKey );

    CString strTree;

    if (pszTreeArg)
        strTree.Format( pszTreeSubKey, pszTreeArg );
    else
        strTree = pszTreeSubKey;

    // Delete the entire registry tree

    long rc = RegDeleteTree( hTreeBase, strTree );

    return (ERROR_SUCCESS == rc || ERROR_FILE_NOT_FOUND == rc);
}

///////////////////////////////////////////////////////////////////////////////
// (generic helpers)

static bool HCRegPut( HKEY hBaseKey, LPCTSTR pszValueName, DWORD dwType, const BYTE* pData, DWORD cbData );
static bool HCRegGet( HKEY hBaseKey, LPCTSTR pszValueName, DWORD dwType, BYTE* pData, DWORD cbData );

///////////////////////////////////////////////////////////////////////////////

bool HCRegPutDWORD( HKEY hBaseKey, LPCTSTR pszValueName, DWORD dwData )
{
    return HCRegPut( hBaseKey, pszValueName, REG_DWORD, (BYTE*) &dwData, sizeof( DWORD ));
}

///////////////////////////////////////////////////////////////////////////////

bool HCRegGetDWORD( HKEY hBaseKey, LPCTSTR pszValueName, DWORD* pdwData )
{
    return HCRegGet( hBaseKey, pszValueName, REG_DWORD, (BYTE*) pdwData, sizeof( DWORD ));
}

///////////////////////////////////////////////////////////////////////////////

bool HCRegPutBINARY( HKEY hBaseKey, LPCTSTR pszValueName, const BYTE* pBin, DWORD dwBinLen )
{
    return HCRegPut( hBaseKey, pszValueName, REG_BINARY, pBin, dwBinLen );
}

///////////////////////////////////////////////////////////////////////////////

bool HCRegGetBINARY( HKEY hBaseKey, LPCTSTR pszValueName, BYTE* pBin, DWORD dwBinLen )
{
    return HCRegGet( hBaseKey, pszValueName, REG_BINARY, pBin, dwBinLen );
}

///////////////////////////////////////////////////////////////////////////////
// Generic helper: dwType == REG_DWORD, REG_BINARY or REG_SZ)
// Note: a NULL 'pszValueName' means "the default value for the key"

static bool HCRegPut( HKEY hBaseKey, LPCTSTR pszValueName, DWORD dwType, const BYTE* pData, DWORD cbData )
{
    ASSERT( hBaseKey && pData && cbData );
    long rc = RegSetValueEx( hBaseKey, pszValueName, 0, dwType, pData, cbData );
    return ERROR_SUCCESS == rc;
}

///////////////////////////////////////////////////////////////////////////////
// Generic helper: dwType == REG_DWORD, or REG_BINARY.
// Don't use for REG_SZ since cbData is indeterminate.

static bool HCRegGet( HKEY hBaseKey, LPCTSTR pszValueName, DWORD dwType, BYTE* pData, DWORD cbData )
{
    ASSERT( hBaseKey && pszValueName && pData && cbData );

    DWORD  dwRegType;
    DWORD  dwRegSize;

    long rc = RegQueryValueEx( hBaseKey, pszValueName, NULL,
                               &dwRegType, pData, &dwRegSize );
    if (0
        || ERROR_SUCCESS != rc
        || dwRegType     != dwType
        || dwRegSize     != cbData
    )
    {
        memset( pData, 0, cbData );
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Note: a NULL 'pszValueName' means "the default value for the key"

bool HCRegPutString( HKEY hBaseKey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
    ASSERT( hBaseKey /* && pszValueName */ && pszValue );
    DWORD dwRegSize = (DWORD) _tcslen( pszValue );
    dwRegSize += 1; // (+NULL string terminator)
    dwRegSize *= sizeof( TCHAR );
    return HCRegPut( hBaseKey, pszValueName, REG_SZ, (BYTE*) pszValue, dwRegSize );
}

///////////////////////////////////////////////////////////////////////////////
// Note: can't use HCRegGet since we don't know how long the string is

bool HCRegGetString( HKEY hBaseKey, LPCTSTR pszValueName, CString& strValue )
{
    ASSERT( hBaseKey && pszValueName );

    LPTSTR  pszString;
    DWORD   dwRegType;
    DWORD   dwRegSize;
    int     nBuffChars;
    long    rc;

    // Keep trying until our buffer is large enough...

    nBuffChars = 1;  // (room for NULL string terminator)

    do
    {
        nBuffChars += MAX_PATH;

        if (!(pszString = strValue.GetBuffer( nBuffChars )))
            break; // (out of memory)

        dwRegSize  = (DWORD) nBuffChars - 1;  // (one less)
        dwRegSize *= sizeof( TCHAR );

        rc = RegQueryValueEx( hBaseKey, pszValueName, NULL,
                              &dwRegType, (BYTE*) pszString, &dwRegSize );
    }
    while (REG_SZ == dwRegType && ERROR_MORE_DATA == rc);

    // Did we get what we expected?

    if (!pszString || REG_SZ != dwRegType || ERROR_SUCCESS != rc)
    {
        strValue.ReleaseBuffer(0);
        strValue.Empty();
        if (!pszString)
            APIFAILEDMSG( strValue.GetBuffer );
        return false;
    }

    // Calculate how many characters we retrieved

    int nRegChars = dwRegSize / sizeof( TCHAR );

    // Ensure string is always NULL terminated!

    // PROGRAMMING NOTE: since we always ask for one less character
    // than the allocated size of our buffer in characters, we know
    // our buffer will always have room for an extra character, so
    // the below is safe and will NOT truncate the returned string.

    ASSERT( pszString && nRegChars && nBuffChars > nRegChars );
    *(pszString + nRegChars) = NULL;
    strValue.ReleaseBuffer( nRegChars );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
