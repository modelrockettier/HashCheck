/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

class CHashCheck : public IShellExtInit, IContextMenu, IShellPropSheetExt, IDropTarget
{
protected:

    VLONG        m_cRef;
    HSIMPLELIST  m_hList;

public:
    CHashCheck() : m_cRef(1), m_hList(0) { IncrementDllReference(); }
    ~CHashCheck()                        { DecrementDllReference();
                                           SLRelease( m_hList );    }

    // IUnknown members

    STDMETHODIMP QueryInterface( REFIID, LPVOID * );
    STDMETHODIMP_( ULONG ) AddRef() { return InterlockedIncrement( &m_cRef ); }
    STDMETHODIMP_( ULONG ) Release()
    {
        // We need a non-volatile variable, hence the cRef variable

        LONG cRef = InterlockedDecrement( &m_cRef );
        if (!cRef)
            delete this;
        return cRef;
    }

    // IShellExtInit members

    STDMETHODIMP Initialize( LPCITEMIDLIST, LPDATAOBJECT, HKEY );

    // IContextMenu members

    STDMETHODIMP QueryContextMenu( HMENU, UINT, UINT, UINT, UINT );
    STDMETHODIMP InvokeCommand( LPCMINVOKECOMMANDINFO );
    STDMETHODIMP GetCommandString( UINT_PTR, UINT, UINT *, LPSTR, UINT );

    // IShellPropSheetExt members

    STDMETHODIMP AddPages( LPFNADDPROPSHEETPAGE, LPARAM );
    STDMETHODIMP ReplacePage( UINT, LPFNADDPROPSHEETPAGE, LPARAM ) { return E_NOTIMPL; }

    // IDropTarget members

    STDMETHODIMP DragEnter( LPDATAOBJECT, DWORD, POINTL, PDWORD )  { return E_NOTIMPL; }
    STDMETHODIMP DragOver( DWORD, POINTL, PDWORD )                 { return E_NOTIMPL; }
    STDMETHODIMP DragLeave()                                       { return E_NOTIMPL; }
    STDMETHODIMP Drop( LPDATAOBJECT, DWORD, POINTL, PDWORD );
};

typedef CHashCheck *LPCHASHCHECK;
