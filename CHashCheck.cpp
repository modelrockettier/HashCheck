/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"
#include "CHashCheck.h"
#include "HashCheckUI.h"
#include "HashCheckOptions.h"

STDMETHODIMP CHashCheck::QueryInterface( REFIID riid, LPVOID *ppv )
{
         if (IsEqualIID( riid, IID_IUnknown           )) *ppv =                       this;
    else if (IsEqualIID( riid, IID_IShellExtInit      )) *ppv = (LPSHELLEXTINIT)      this;
    else if (IsEqualIID( riid, IID_IContextMenu       )) *ppv = (LPCONTEXTMENU)       this;
    else if (IsEqualIID( riid, IID_IShellPropSheetExt )) *ppv = (LPSHELLPROPSHEETEXT) this;
    else if (IsEqualIID( riid, IID_IDropTarget        )) *ppv = (LPDROPTARGET)        this;
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP CHashCheck::Initialize( LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdtobj, HKEY hkeyProgID )
{
    // We'll be needing a buffer, and let's double it just to be safe

    TCHAR  szPath[ MAX_PATH << 1 ];

    // Make sure that we are working with a fresh list

    SLRelease( m_hList );
    m_hList = SLCreate(FALSE);

    // This indent exists to facilitate diffing against the CmdOpen source

    FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium;

    if (!pdtobj || pdtobj->GetData( &format, &medium ) != S_OK)
        return E_INVALIDARG;

    HDROP hDrop = (HDROP) GlobalLock( medium.hGlobal );

    if (hDrop)
    {
        UINT uDrop, uDrops = DragQueryFile( hDrop, -1, NULL, 0 );

        for (uDrop = 0; uDrop < uDrops; ++uDrop)
        {
            if (DragQueryFile( hDrop, uDrop, szPath, _countof( szPath )))
                SLAddStringI( m_hList, szPath );
        }

        GlobalUnlock( medium.hGlobal );
    }

    ReleaseStgMedium( &medium );


    // If there was any failure, the list would be empty...

    return SLCheck( m_hList ) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CHashCheck::QueryContextMenu( HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
{
    if (uFlags & (CMF_DEFAULTONLY | CMF_NOVERBS))
        return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, 0 );

    // Load the menu display settings

    HASHOPTIONS opt;  opt.dwFlags = HCOF_MENUDISPLAY;

    OptionsLoad( &opt );

    // Do not show if the settings prohibit it

    if (opt.dwMenuDisplay == 2 || (opt.dwMenuDisplay == 1 && !(uFlags & CMF_EXTENDEDVERBS)))
        return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, 0 );

    // Load the localized menu text

    TCHAR  szMenuText[ MAX_STRINGMSG ];

    LoadString( g_hModThisDll, IDS_HS_MENUTEXT, szMenuText, _countof( szMenuText ));

    if (InsertMenu( hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst, szMenuText ))
        return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, 1 );

    return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, 0 );
}

STDMETHODIMP CHashCheck::InvokeCommand( LPCMINVOKECOMMANDINFO pici )
{
    // Ignore string verbs (high word must be zero)
    // The only valid command index is 0 (low word must be zero)

    if (pici->lpVerb)
        return E_INVALIDARG;

    // Hand things over to HashSave, where all the work is done...

    HashSaveStart( pici->hwnd, m_hList );

    // HaveSave has AddRef'ed and now owns our list

    SLRelease( m_hList );
    m_hList = NULL;

    return S_OK;
}

STDMETHODIMP CHashCheck::GetCommandString( UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax )
{
    static const CString            strVerb(_T("cksum"));
    static const CStringA strVerbA( strVerb );
    static const CStringW strVerbW( strVerb );

    if (idCmd || cchMax < (UINT) strVerb.GetLength())
        return E_INVALIDARG;

    switch (uFlags)
    {
        case GCS_VERBA:
        {
            strcpy_s( (LPSTR) pszName, cchMax, strVerbA );
            return S_OK;
        }

        case GCS_VERBW:
        {
            wcscpy_s( (LPWSTR) pszName, cchMax, strVerbW );
            return S_OK;
        }
    }

    // The help text (status bar text) should not contain any of the
    // characters added for the menu access keys.

    switch (uFlags)
    {
        case GCS_HELPTEXTA:
        {
            CStringA strMenuTextA;
            VERIFY(  strMenuTextA.LoadString( g_hModThisDll, IDS_HS_MENUTEXT ));
                     strMenuTextA.Replace("&","");
            strcpy_s( (LPSTR) pszName, cchMax, strMenuTextA );
            return S_OK;
        }

        case GCS_HELPTEXTW:
        {
            CStringW strMenuTextW;
            VERIFY(  strMenuTextW.LoadString( g_hModThisDll, IDS_HS_MENUTEXT ));
                     strMenuTextW.Replace(L"&",L"");
            wcscpy_s( (LPWSTR) pszName, cchMax, strMenuTextW );
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

STDMETHODIMP CHashCheck::AddPages( LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam )
{
    PROPSHEETPAGE  psp;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USECALLBACK | PSP_USEREFPARENT | PSP_USETITLE;
    psp.hInstance   = g_hModThisDll;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_HASHPROP);
    psp.pszTitle    = MAKEINTRESOURCE(IDS_HP_TITLE);
    psp.pfnDlgProc  = HashPropDlgProc;
    psp.lParam      = (LPARAM)m_hList;
    psp.pfnCallback = HashPropCallback;
    psp.pcRefParent = (UINT*) PointerToDllReference();

    if (ActivateManifest( FALSE ))
    {
        psp.dwFlags |= PSP_USEFUSIONCONTEXT;
        psp.hActCtx = g_hActCtx;
    }

    HPROPSHEETPAGE  hPage  = CreatePropertySheetPage( &psp );

    if (hPage && !pfnAddPage( hPage, lParam ))
        DestroyPropertySheetPage( hPage );

    // HashProp has AddRef'ed and now owns our list

    SLRelease( m_hList );
    m_hList = NULL;

    return S_OK;
}

STDMETHODIMP CHashCheck::Drop( LPDATAOBJECT pdtobj, DWORD grfKeyState, POINTL pt, PDWORD pdwEffect )
{
    FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium;

    UINT uThreads = 0;

    if (pdtobj && pdtobj->GetData( &format, &medium ) == S_OK)
    {
        if (HDROP hDrop = (HDROP) GlobalLock( medium.hGlobal ))
        {
            UINT uDrops = DragQueryFile( hDrop, -1, NULL, 0 );
            UINT cchPath;
            LPTSTR lpszPath;

            // Reduce the likelihood of a race condition when trying to create
            // an activation context by creating it before creating threads

            ActivateManifest( FALSE );

            for (UINT uDrop = 0; uDrop < uDrops; ++uDrop)
            {
                if (1
                    && (cchPath = DragQueryFile( hDrop, uDrop, NULL, 0 ))
                    && (lpszPath = (LPTSTR) malloc( (cchPath + 1) * sizeof( TCHAR )))
                )
                {
                    IncrementDllReference();

                    HANDLE  hThread;

                    if (1
                        && DragQueryFile( hDrop, uDrop, lpszPath, cchPath+1 ) == cchPath
                        && !(GetFileAttributes( lpszPath ) & FILE_ATTRIBUTE_DIRECTORY)
                        && (hThread = CreateWorkerThread( HashVerifyThread, lpszPath ))
                    )
                    {
                        // The thread should free lpszPath, not us

                        CloseHandle( hThread );
                        ++uThreads;
                    }
                    else
                    {
                        // Something went wrong?

                        free( lpszPath );
                        DecrementDllReference();
                    }
                }
            }

            GlobalUnlock( medium.hGlobal );
        }

        ReleaseStgMedium( &medium );
    }

    if (!uThreads)
    {
        // We shouldn't ever be hitting this case

        *pdwEffect = DROPEFFECT_NONE;
        ASSERT( false );
        return E_INVALIDARG;
    }

    // DROPEFFECT_LINK would work here as well; it really doesn't matter

    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}
