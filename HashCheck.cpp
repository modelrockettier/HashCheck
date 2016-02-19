/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"
#include "HashCheckOptions.h"

#if defined(_USRDLL) && defined(_DLL)
  #pragma comment( linker, "/entry:DllMain" )
#endif

///////////////////////////////////////////////////////////////////////////////
//
//                  "DllMain" is our DLL's entry-point
//
///////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, void* pReserved )
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
#ifdef _DLL
        DisableThreadLibraryCalls( hInstance );
#endif
        return InitializeDll( hInstance );
    }

    if (DLL_PROCESS_DETACH == dwReason)
    {
        if (1
            && g_bActCtxCreated
            && INVALID_HANDLE_VALUE != g_hActCtx
        )
            ReleaseActCtx( g_hActCtx );

        return TRUE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Required entry-point: Check whether or not our DLL may be safely unloaded.

STDAPI DllCanUnloadNow()
{
    LONG cRef = CurrentDllReference();      // (grab a non-volatile copy)
    HRESULT hr = (0 == cRef) ? S_OK         // (if no refs, OK to unload)
                             : S_FALSE;     // (else must NOT unload yet)

    HCTRACE( _T("cRef = %d, hr = %s\n"), cRef, HResultMessage( hr ));
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Required entry-point: Returns the class factory.

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppv )
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    *ppv = NULL;

    if (IsEqualIID( rclsid, PRODUCT_CLSID_GUID ))
    {
        CHashCheckClassFactory*  pHashCheckClassFactory  = new CHashCheckClassFactory;

        if (pHashCheckClassFactory)
        {
            hr = pHashCheckClassFactory->QueryInterface( riid, ppv );
            pHashCheckClassFactory->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }

    HCTRACE(_T("hr = %s\n"), HResultMessage( hr ));
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Check to make sure we're running elevated

static bool RunningElevated()
{
    bool bElevated = false;
    HANDLE hToken = NULL;

    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
    {
        TOKEN_ELEVATION        tkElevation;
        DWORD cbSize = sizeof( tkElevation );

        if (GetTokenInformation( hToken, TokenElevation, &tkElevation, cbSize, &cbSize ))
            bElevated = tkElevation.TokenIsElevated ? true : false;
    }

    if (hToken)
        CloseHandle( hToken );

    if (!bElevated)
    {
        // "The requested operation requires elevation." (in their own language)
        CString strErrMsg = FormatLastError( ERROR_ELEVATION_REQUIRED );
        MessageBox( NULL, strErrMsg, NULL, MB_OK | MB_ICONERROR );
    }

    return bElevated;
}

///////////////////////////////////////////////////////////////////////////////

static bool RegisterHCClass()
{
    HKEY     hKey;
    bool     bSuccess;
    CString  strProductCLSIDKey;
    CString  strFullPathDll = FullPathDll();

    strProductCLSIDKey.Format( _T("CLSID\\%s"), PRODUCT_CLSID );

    if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, strProductCLSIDKey )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }

    bSuccess = HCRegPutString( hKey, NULL, PRODUCT_DESC );
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );

    if (bSuccess)
    {
        CString  strProductServer32Key  = strProductCLSIDKey + _T("\\InprocServer32");

        if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, strProductServer32Key )))
        {
            APIFAILEDMSG( HCRegCreateOpen );
            return false;
        }

        bSuccess =
        (1
            && HCRegPutString( hKey,          NULL,         strFullPathDll )
            && HCRegPutString( hKey, _T("ThreadingModel"), _T("Apartment") )
        );
        if (!bSuccess)
            APIFAILEDMSG( HCRegPutString );
        RegCloseKey( hKey );
    }

    return bSuccess;
}

//-----------------------------------------------------------------------------

static bool UnRegisterHCClass()
{
    CString  strProductCLSIDKey;
    strProductCLSIDKey.Format( _T("CLSID\\%s"), PRODUCT_CLSID );
    bool bSuccess = HCRegDeleteTree( HKEY_CLASSES_ROOT, strProductCLSIDKey );
    if (!bSuccess)
        APIFAILEDMSG( HCRegDeleteTree );
    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////

static bool RegisterHCShellHandlers()
{
    bool     bSuccess = true;
    CString  strOurHandlerKey;
    HKEY     hOurHandlerKey;

    strOurHandlerKey = _T("AllFileSystemObjects\\ShellEx")
        _T("\\ContextMenuHandlers\\") PRODUCT_DESC;

    if (!(hOurHandlerKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, strOurHandlerKey )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }

    bSuccess = HCRegPutString( hOurHandlerKey, NULL, PRODUCT_CLSID ) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hOurHandlerKey );

    strOurHandlerKey = _T("AllFileSystemObjects\\ShellEx")
        _T("\\PropertySheetHandlers\\") PRODUCT_DESC;

    if (!(hOurHandlerKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, strOurHandlerKey )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }

    bSuccess = HCRegPutString( hOurHandlerKey, NULL, PRODUCT_CLSID ) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hOurHandlerKey );

    return bSuccess;
}

//-----------------------------------------------------------------------------

static bool UnRegisterHCShellHandlers()
{
    bool     bSuccess = true;
    CString  strAllFileSysObjShellEx  = _T("AllFileSystemObjects\\ShellEx");
    HKEY     hShellExKey;

    if (!(hShellExKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, strAllFileSysObjShellEx )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }

    CString strOurHandlerKey;

    strOurHandlerKey = _T("ContextMenuHandlers") _T("\\") PRODUCT_DESC;
    bSuccess = HCRegDeleteTree( hShellExKey, strOurHandlerKey ) && bSuccess;

    if (!bSuccess)
        APIFAILEDMSG( HCRegDeleteTree );

    strOurHandlerKey = _T("PropertySheetHandlers") _T("\\") PRODUCT_DESC;
    bSuccess = HCRegDeleteTree( hShellExKey, strOurHandlerKey ) && bSuccess;

    if (!bSuccess)
        APIFAILEDMSG( HCRegDeleteTree );

    RegCloseKey( hShellExKey );

    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////

static bool RegisterHCProgramId()
{
    bool     bSuccess        = true;
    CString  strFullPathDll  = FullPathDll();
    HKEY     hKey;

    // It all starts here:  HKEY_CLASSES_ROOT + "HashCheck" ...

    if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, PRODUCT_NAME )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }

    CString  strFileTypeDesc;
    CString  strValue;
    CString  strAppUserModelId  = APP_USER_MODEL_ID;

    // (retrieve localized description of our file types = "Checksum File")

    if (!strFileTypeDesc.LoadString( g_hModThisDll, IDS_FILETYPE_DESC ))
    {
        APIFAILEDMSG( LoadString );
        RegCloseKey( hKey );
        return false;
    }

    bSuccess = HCRegPutString( hKey, NULL, strFileTypeDesc ) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );

    // miscellaneous shell values

    strValue.Format( _T("@\"%s\",-%u"), strFullPathDll, IDS_FILETYPE_DESC );

    bSuccess = HCRegPutString( hKey, _T("FriendlyTypeName"),    strValue     ) && bSuccess;
    bSuccess = HCRegPutString( hKey, _T("AlwaysShowExt"),        _T("")      ) && bSuccess;
    bSuccess = HCRegPutString( hKey, _T("AppUserModelID"), strAppUserModelId ) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );

    // default icon for our file types

    if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, _T("%s\\DefaultIcon"), PRODUCT_NAME )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }
    strValue.Format( _T("%s,-%u"), strFullPathDll, IDI_FILETYPE );
    bSuccess = HCRegPutString( hKey, NULL, strValue ) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );

    // default shell command = open

    if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, _T("%s\\shell"), PRODUCT_NAME )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }
    bSuccess = HCRegPutString( hKey, NULL, _T("open")) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );

    // DropTarget (CHashCheck::Drop) is the primary manner that we're invoked.
    // The "HashCheck_VerifyW" function is an alternate legacy entry-point function
    // that accomplishes the same thing: both call the "HashVerifyThread" passing
    // it the name of the file whose checksum (hash) is to be verified.

    if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, _T("%s\\shell\\open\\DropTarget"), PRODUCT_NAME )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }
    bSuccess = HCRegPutString( hKey, _T("CLSID"), PRODUCT_CLSID ) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );

    // legacy open command in case DropTarget isn't supported

    if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, _T("%s\\shell\\open\\command"), PRODUCT_NAME )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }
    strValue.Format( _T("rundll32.exe \"%s\",HashCheck_VerifyW %%1"), strFullPathDll );
    bSuccess = HCRegPutString( hKey, NULL, strValue ) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );

    // default editor for our file types = notepad

    if (!(hKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, _T("%s\\shell\\edit\\command"), PRODUCT_NAME )))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }
    bSuccess = HCRegPutString( hKey, NULL, _T("notepad.exe %1")) && bSuccess;
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );

    return bSuccess;
}

//-----------------------------------------------------------------------------

static bool UnRegisterHCProgramId()
{
    bool bSuccess = HCRegDeleteTree( HKEY_CLASSES_ROOT, PRODUCT_NAME );
    if (!bSuccess)
        APIFAILEDMSG( HCRegDeleteTree );
    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// Associate our class with our CLSID as an approved shell extension

static bool RegisterHCShellExtensionApproval()
{
    HKEY hKey;
    if (!(hKey = HCRegCreateOpen( HKEY_LOCAL_MACHINE,
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"))))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }
    bool bSuccess = HCRegPutString( hKey, PRODUCT_CLSID, PRODUCT_DESC );
    if (!bSuccess)
        APIFAILEDMSG( HCRegPutString );
    RegCloseKey( hKey );
    return bSuccess;
}

//-----------------------------------------------------------------------------

static bool UnRegisterHCShellExtensionApproval()
{
    HKEY hKey;
    if (!(hKey = HCRegCreateOpen( HKEY_LOCAL_MACHINE,
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"))))
    {
        APIFAILEDMSG( HCRegCreateOpen );
        return false;
    }
    bool bSuccess = HCRegDeleteValue( hKey, PRODUCT_CLSID );
    if (!bSuccess)
        APIFAILEDMSG( HCRegDeleteValue );
    RegCloseKey( hKey );
    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// Define our class as the default handler for our supported file extensions

static bool RegisterHCFileExtensions()
{
    bool bSuccess = true;
    HKEY hFileExtKey, hHandlerKey;
    int i;

    for (i=0; bSuccess && i < _countof( g_szHashExtsTab ); ++i)
    {
        if (!(hFileExtKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, g_szHashExtsTab[i] )))
        {
            APIFAILEDMSG( HCRegCreateOpen );
            bSuccess = false;
        }
        else
        {
            bSuccess = (1
                && HCRegPutString( hFileExtKey, NULL, PRODUCT_NAME )
                && HCRegPutString( hFileExtKey, _T("PerceivedType"), _T("text"))
                && bSuccess
            );
            if (!bSuccess)
                APIFAILEDMSG( HCRegPutString );

            if (!(hHandlerKey = HCRegCreateOpen( hFileExtKey, _T("PersistentHandler"))))
            {
                APIFAILEDMSG( HCRegCreateOpen );
                bSuccess = false;
            }
            else
            {
                bSuccess = HCRegPutString( hHandlerKey, NULL, PRODUCT_CLSID );
                if (!bSuccess)
                    APIFAILEDMSG( HCRegPutString );
                RegCloseKey( hHandlerKey );
            }

            RegCloseKey( hFileExtKey );
        }
    }
    return bSuccess;
}

//-----------------------------------------------------------------------------

static bool UnRegisterHCFileExtensions()
{
    bool bSuccess = true;
    HKEY hFileExtKey;
    int i;

    for (i=0; i < _countof( g_szHashExtsTab ); ++i)
    {
        hFileExtKey = HCRegCreateOpen( HKEY_CLASSES_ROOT, g_szHashExtsTab[i] );

        if (!hFileExtKey)
        {
            APIFAILEDMSG( HCRegCreateOpen );
            bSuccess = false;
        }
        else
        {
            RegCloseKey( hFileExtKey );
            if (!HCRegDeleteTree( HKEY_CLASSES_ROOT, g_szHashExtsTab[i] ))
            {
                APIFAILEDMSG( HCRegDeleteTree );
                bSuccess = false;
            }
        }
    }
    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// Add ourselves to the "Add/Remove Programs" database

static bool RegisterHCARP()
{
    CString strHCAddRemovePgmsKey =
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
    strHCAddRemovePgmsKey += PRODUCT_CLSID;

    bool bSuccess = HCRegDeleteTree( HKEY_LOCAL_MACHINE, strHCAddRemovePgmsKey );
    if (!bSuccess)
        APIFAILEDMSG( HCRegDeleteTree );

    HKEY hKey = HCRegCreateOpen( HKEY_LOCAL_MACHINE, strHCAddRemovePgmsKey );
    bSuccess = hKey ? true : false;
    if (!bSuccess)
        APIFAILEDMSG( HCRegCreateOpen );
    else
    {
        CString strPublisher   = _T("Kai Liu");
        CString strURL         = _T("http://code.kliu.org/");
        CString strHelp        = _T("http://code.kliu.org/hashcheck/");
        CString strName        = PRODUCT_DESC _T(", ") ARCH_NAME;
        CString strVersion     = VERSION_STR;
        CString strFullPathDll = FullPathDll();
        CString strUninstallerCmdline;

        strUninstallerCmdline.Format( _T("regsvr32.exe /u \"%s\""),
            strFullPathDll );

        bSuccess = HCRegPutString( hKey, _T("DisplayName"),     strName )               && bSuccess;
        bSuccess = HCRegPutString( hKey, _T("DisplayVersion"),  strVersion )            && bSuccess;
        bSuccess = HCRegPutString( hKey, _T("DisplayIcon"),     strFullPathDll )        && bSuccess;
        bSuccess = HCRegPutString( hKey, _T("Publisher"),       strPublisher )          && bSuccess;
        bSuccess = HCRegPutString( hKey, _T("HelpLink"),        strHelp )               && bSuccess;
        bSuccess = HCRegPutString( hKey, _T("URLInfoAbout"),    strURL )                && bSuccess;
        bSuccess = HCRegPutDWORD(  hKey, _T("NoModify"),        1 )                     && bSuccess;
        bSuccess = HCRegPutDWORD(  hKey, _T("NoRepair"),        1 )                     && bSuccess;
        bSuccess = HCRegPutString( hKey, _T("UninstallString"), strUninstallerCmdline ) && bSuccess;

        if (!bSuccess)
            APIFAILEDMSG( HCRegPutString );

        RegCloseKey( hKey );
    }
    return bSuccess;
}

//-----------------------------------------------------------------------------

static bool UnRegisterHCARP()
{
    CString strHCAddRemovePgmsKey =
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
    strHCAddRemovePgmsKey += PRODUCT_CLSID;
    bool bSuccess = HCRegDeleteTree( HKEY_LOCAL_MACHINE, strHCAddRemovePgmsKey );
    if (!bSuccess)
        APIFAILEDMSG( HCRegDeleteTree );
    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// Extract embedded resource

static bool ExtractEmbeddedResource( UINT nResId, LPCTSTR pszPath )
{
    // (load the resource...)

    HRSRC    hSrc;    // handle to resource information
    HGLOBAL  hRes;    // handle to loaded resource
    void*    pRes;    // ptr to resource data
    DWORD    dwSz;    // size of resource data

    if (!(hSrc = FindResource( g_hModThisDll,
        MAKEINTRESOURCE( nResId ), RT_RCDATA )))
    {
        APIFAILEDMSG( FindResource );
        return false;
    }

    if (!(hRes = LoadResource( g_hModThisDll, hSrc )))
    {
        APIFAILEDMSG( LoadResource );
        return false;
    }

    if (!(pRes = LockResource( hRes )))
    {
        APIFAILEDMSG( LockResource );
        return false;
    }

    if (!(dwSz = SizeofResource( g_hModThisDll, hSrc )))
    {
        APIFAILEDMSG( SizeofResource );
        return false;
    }

    // (write out the resource...)

    HANDLE hFile = CreateFile
    (
        pszPath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY,
        NULL
    );

    if (INVALID_HANDLE_VALUE == hFile )
    {
        APIFAILEDMSG( CreateFile_EXE );
        return false;
    }

    DWORD dwWritten;

    if (!WriteFile( hFile, pRes, dwSz, &dwWritten, NULL )
        || dwWritten != dwSz)
    {
        APIFAILEDMSG( WriteFile_EXE );
        VERIFY( CloseHandle( hFile ));
        return false;
    }

    VERIFY( CloseHandle( hFile ));
    hFile = NULL;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Hopefully our "RemoveDLL()" function won't take any longer than this.

#define HC_MAX_REMOVEDLL_TIME   1000    // (hopefully long enough!)

///////////////////////////////////////////////////////////////////////////////
// Notify the shell about our INSTALL.
// Used by DllRegisterServer.

static bool NotifyShellOfInstall()
{
    //  "Applications that register new handlers of any type must call
    //   SHChangeNotify with the SHCNE_ASSOCCHANGED flag to instruct
    //   the Shell to invalidate the cache and search for new handlers"

    CoFreeUnusedLibrariesEx( HC_MAX_REMOVEDLL_TIME, 0 );
    SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL );
    CoFreeUnusedLibrariesEx( HC_MAX_REMOVEDLL_TIME, 0 );
    SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Notify the shell about our UNinstall.
// Used by DllUnregisterServer.

static bool NotifyShellOfUninstall()
{
    CoFreeUnusedLibrariesEx( HC_MAX_REMOVEDLL_TIME, 0 );
    SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL );
    CoFreeUnusedLibrariesEx( HC_MAX_REMOVEDLL_TIME, 0 );
    SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Remove our DLL from the system.  Used by DllUnregisterServer.

static bool RemoveDLL()
{
    bool bSuccess = true;

    CPath  strFullPathDll  = FullPathDll();
    CPath  strTempDir      = TempPath( true );
    CPath  strDllName      = TempName( strTempDir );

    // First, try physically MOVING (not copying!) the DLL
    // to another directory and then deleting it from there.

    HCTRACE(_T("Move \"%s\" to \"%s\" ...\n"),
        strFullPathDll, strDllName );

    if (!MoveFileEx( strFullPathDll, strDllName,
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ))
    {
        APIFAILEDMSG( MoveFileEx );
        return false;
    }

    // Now delete it from where we moved it to. We do this by extracting
    // our small utility program which is embedded within ourselves as a
    // binary resource and using it to refresh the shell and then delete
    // ourselves afterwards. We invoke it via a small batch program that
    // we create which not only deletes the utility program too but also
    // itself when it's done, thereby leaving no leftover crumbs at all.
    // (At least in theory anyway!)

    CString  strCmdName  = TempName( strTempDir );
    VERIFY( DeleteFile( strCmdName ));
    strCmdName += _T(".cmd");
    HCTRACE(_T("strCmdName = \"%s\"\n"), strCmdName );

    if (!ExtractEmbeddedResource( IDC_EMBEDDED_CMD_RESID, strCmdName ))
        return false;

    CString  strExeName  = TempName( strTempDir );
    VERIFY( DeleteFile( strExeName ));
    strExeName += _T(".exe");
    HCTRACE(_T("strExeName = \"%s\"\n"), strExeName );

    if (!ExtractEmbeddedResource( IDC_EMBEDDED_EXE_RESID, strExeName ))
        return false;

    // (execute batch file...)

    CString  strCmdLine;
    strCmdLine.Format(_T("\"%s\" \"%s\" \"%s\" %d"),
        strCmdName,
        strExeName,
        strDllName,
        HC_MAX_REMOVEDLL_TIME );
    HCTRACE(_T("strCmdLine = %s\n"), strCmdLine );

    STARTUPINFO          si  =  { sizeof( si ), 0 };
    PROCESS_INFORMATION  pi  =  {0};

    si.dwFlags     = STARTF_USESHOWWINDOW;  // (hidden)
    si.wShowWindow = SW_HIDE;               // (hidden)

    // PROGRAMMING NOTE: the documentation for CreateProcess states that
    // the command-line is modified and thus needs to point to writable
    // storage (i.e. it can't be LPCTSTR, it must be LPTSTR). To be safe
    // (since it will be modified) we also include some extra space too.

    strCmdLine += _T("        ");
    LPTSTR  pszCmdLine  = strCmdLine.GetBuffer( strCmdLine.GetLength() + 8 );

    if (!CreateProcess( 0, pszCmdLine, 0, 0, 0, 0, 0, 0, &si, &pi ))
    {
        APIFAILEDMSG( CreateProcess );
        return false;
    }

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    strCmdLine.ReleaseBuffer();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// DllRegisterServer/DllUnregisterServer helper macro and function to
// log the success/failure of each register/unregister function called.
// (Note: we test 'bSuccess' first to abort further processing.)

#define REGUNREGCALL( FUNC ) \
    (bSuccess && (bSuccess = HCLogRegUnregFunc( &FUNC, _T(#FUNC))))

typedef bool HCREGUNREGFUNC();

static bool HCLogRegUnregFunc( HCREGUNREGFUNC* pfn, LPCTSTR pszFunc )
{
    register bool bSuccess = pfn();
    TRACE( _T("*** %s : %s : %s\n"), _T(TARGETNAME), pszFunc,
        bSuccess ? _T("Success.") : _T("FAILED!") );
    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// Required entry-point: INSTALL our shell extension.

STDAPI DllRegisterServer()
{
    // PROGRAMMING NOTE: it's important to add the "Add/Remove Programs"
    // registry entries FIRST in case anything goes wrong later on. That
    // way if something does go wrong, the user at least has the chance
    // of trying to "Undo" (backout) whatever changes did happen to have
    // been made during their attempt by simply performing an uninstall.

    bool bSuccess = true;   // (needed by REGUNREGCALL)

    HRESULT hr =
    (1
        && REGUNREGCALL( RunningElevated )
        && REGUNREGCALL( RegisterHCARP )
        && REGUNREGCALL( RegisterHCClass )
        && REGUNREGCALL( RegisterHCProgramId )
        && REGUNREGCALL( RegisterHCShellHandlers )
        && REGUNREGCALL( RegisterHCShellExtensionApproval )
        && REGUNREGCALL( RegisterHCFileExtensions )
        && REGUNREGCALL( NotifyShellOfInstall )
    )
    ? S_OK                  // (success)
    : SELFREG_E_CLASS;      // (failure)

    HCTRACE(_T("hr = %s\n\n"), HResultMessage( hr ));
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Required entry-point: UNINSTALL our shell extension.

STDAPI DllUnregisterServer()
{
    // PROGRAMMING NOTE: it's important to remove the "Add/Remove Programs"
    // registry entries LAST in case anything goes wrong during the process.
    // Then if something does happen to go wrong, the user can then at least
    // retry their uninstall attempt again because the "Add/Remove Programs"
    // registry entries would therefore still exist.

    bool bSuccess = true;   // (needed by REGUNREGCALL)

    HRESULT hr =
    (1
        && REGUNREGCALL( RunningElevated )
        && REGUNREGCALL( UnRegisterHCFileExtensions )
        && REGUNREGCALL( UnRegisterHCProgramId )
        && REGUNREGCALL( UnRegisterHCShellHandlers )
        && REGUNREGCALL( UnRegisterHCShellExtensionApproval )
        && REGUNREGCALL( UnRegisterHCClass )
        && REGUNREGCALL( OptionsDelete )
        && REGUNREGCALL( UnRegisterHCARP )
        && REGUNREGCALL( NotifyShellOfUninstall )
        && REGUNREGCALL( RemoveDLL )
    )
    ? S_OK                  // (success)
    : SELFREG_E_CLASS;      // (failure)

    HCTRACE(_T("hr = %s\n\n"), HResultMessage( hr ));
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Required entry-point: Install/Uninstall our shell extension.

STDAPI DllInstall( BOOL bInstall, LPCWSTR pszCmdLine )
{
    //---------------------------------------------------------------
    //                        HashCheck
    //---------------------------------------------------------------
    //
    // To install:
    //
    //    1. Copy DLL to %SystemRoot%\system32
    //
    //    2. Open an ADMINISTRATOR Command Prompt, navigate to the
    //       %SystemRoot%\system32 directory, and enter the command:
    //
    //       regsvr32.exe "%SystemRoot%\system32\HashCheck.dll"
    //
    //    3. Logoff and then logon again to restart the shell.
    //
    //
    // To uninstall:
    //
    //    1. Open an ADMINISTRATOR Command Prompt, navigate to the
    //       %SystemRoot%\system32 directory, and enter the command:
    //
    //       regsvr32.exe /u "%SystemRoot%\system32\HashCheck.dll"
    //
    //    2. Logoff and then logon again to restart the shell.
    //
    //
    // Note: specifying the full path in the regsvr32 command is
    // TECHNICALLY not necessary, but it's safer since without it,
    // regsvr32 will load/call the first instance it finds anywhere
    // in the search PATH which could load/call the wrong DLL.
    //
    //---------------------------------------------------------------

    HRESULT hr = bInstall ? DllRegisterServer() : DllUnregisterServer();
    HCTRACE(_T("hr = %s\n\n"), HResultMessage( hr ));
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
