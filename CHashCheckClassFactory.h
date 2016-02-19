/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

///////////////////////////////////////////////////////////////////////////////

class CHashCheckClassFactory : public IClassFactory
{
protected:

    VLONG  m_cRef;      // (reference count)

public:

    CHashCheckClassFactory()
        : m_cRef (1)
    {
        IncrementDllReference();
    }

    ~CHashCheckClassFactory()
    {
        DecrementDllReference();
    }

    // IClassFactory members

    STDMETHODIMP  CreateInstance( LPUNKNOWN, REFIID, LPVOID* );
    STDMETHODIMP  LockServer( BOOL )
    {
        return E_NOTIMPL;
    }

    // IUnknown members

    STDMETHODIMP  QueryInterface( REFIID, LPVOID* );

    STDMETHODIMP_(ULONG)  AddRef()
    {
        return InterlockedIncrement( &m_cRef );
    }

    STDMETHODIMP_(ULONG)  Release()
    {
        // PROGRAMMING NOTE: we must be careful to *NOT* use
        // our 'm_cRef' variable in the below "if" statement
        // and *NOT* return its value either, since it could
        // change at any moment.  Rather, we must *ONLY* use
        // the value returned to us by InterlockedDecrement.

        LONG cRef = InterlockedDecrement( &m_cRef );
        if (!cRef)
            delete this;
        return cRef;
    }
};

///////////////////////////////////////////////////////////////////////////////
