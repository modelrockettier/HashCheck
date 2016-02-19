/**
 * Registry Helper Functions
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

///////////////////////////////////////////////////////////////////////////////

HKEY  HCRegCreateOpen  ( HKEY hBaseKey,  LPCTSTR pszSubKey,     LPCTSTR pszSubKeyArg    = NULL );
bool  HCRegDeleteValue ( HKEY hBaseKey,  LPCTSTR pszValueName,  LPCTSTR pszValueNameArg = NULL );
bool  HCRegDeleteTree  ( HKEY hTreeBase, LPCTSTR pszTreeSubKey, LPCTSTR pszTreeArg      = NULL );

bool  HCRegPutString   ( HKEY hBaseKey, LPCTSTR pszValueName, LPCTSTR  pszValue );
bool  HCRegGetString   ( HKEY hBaseKey, LPCTSTR pszValueName, CString& strValue );

bool  HCRegPutDWORD    ( HKEY hBaseKey, LPCTSTR pszValueName, DWORD   dwData );
bool  HCRegGetDWORD    ( HKEY hBaseKey, LPCTSTR pszValueName, DWORD* pdwData );

///////////////////////////////////////////////////////////////////////////////
