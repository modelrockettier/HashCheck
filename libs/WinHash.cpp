/**
 * Windows Hashing/Checksumming Library
 * Last modified: 2009/01/13
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This is a wrapper for the MD4, MD5, SHA1 and SHA256 algorithms
 * supported natively by the Windows API. The CRC32 algorithm is
 * supported via our builtin CRC32 table driven library interface.
 *
 * WinHash.c is needed only if the WH*To* or WH*Ex functions are used.
 **/

#include "stdafx.h"
#include "WinHash.h"

///////////////////////////////////////////////////////////////////////////////
//
//                 Table of supported file extensions
//
///////////////////////////////////////////////////////////////////////////////

LPCTSTR  g_szHashExtsTab[ NUM_HASH_TYPES ] =
{
   HASH_EXT_CRC32,
   HASH_EXT_MD4,
   HASH_EXT_MD5,
   HASH_EXT_SHA1,
   HASH_EXT_SHA256
};

///////////////////////////////////////////////////////////////////////////////
//
//                          WH*Ex functions
//
///////////////////////////////////////////////////////////////////////////////

static CONDITION_VARIABLE  g_cvUpdate;              // update condition
static SRWLOCK             g_srwLock;               // update "lock"
static const BYTE*         g_pbIn      = NULL;      // update data
static UINT                g_cbIn      = 0;         // update length
static LONG                g_lCtr      = 0;         // done counter
static HANDLE              g_hDone     = NULL;      // done event
static volatile bool       g_bFinish   = false;     // finish flag

//-----------------------------------------------------------------------------

#define SIGNAL_DONE()                                           \
                                                                \
    do {                                                        \
        if (InterlockedIncrement( &g_lCtr ) >= NUM_HASH_TYPES)  \
            VERIFY( SetEvent( g_hDone ));                       \
    } while (0)

//-----------------------------------------------------------------------------

typedef void WHAPI WHInitFunc  ( void* pContext );
typedef void WHAPI WHUpdateFunc( void* pContext, const BYTE* pbIn, UINT cbIn );
typedef void WHAPI WHFinishFunc( void* pContext );

struct HASHPARMS
{
    WHInitFunc*    pInitFunc;       // initialization function
    WHUpdateFunc*  pUpdateFunc;     // update function
    WHFinishFunc*  pFinishFunc;     // finish function

    void*          pCTX;            // pointer to hash context
    UINT8          flag;            // hash type flags
    UINT8          type;            // context type
    UINT8          rcase;           // hash results case

    BYTE*          pBinHash;        // binary hash results buffer
    UINT           cbBinHash;       // binary hash results length
    LPTSTR         pszCharHash;     // char hash results buffer
    UINT           cbCharHash;      // char hash results length

};
typedef struct HASHPARMS HASHPARMS;

static HASHPARMS CRC32_Parms  = { (WHInitFunc*) &WHInitCRC32,  (WHUpdateFunc*) &WHUpdateCRC32,  (WHFinishFunc*) &WHFinishCRC32,  NULL, 0, WHCTX_FLAG_CRC32,  0, NULL, HASH_BLEN_CRC32,  NULL, HASH_CLEN_CRC32  + 1 };
static HASHPARMS MD4_Parms    = { (WHInitFunc*) &WHInitMD4,    (WHUpdateFunc*) &WHUpdateMD4,    (WHFinishFunc*) &WHFinishMD4,    NULL, 0, WHCTX_FLAG_MD4,    0, NULL, HASH_BLEN_MD4,    NULL, HASH_CLEN_MD4    + 1 };
static HASHPARMS MD5_Parms    = { (WHInitFunc*) &WHInitMD5,    (WHUpdateFunc*) &WHUpdateMD5,    (WHFinishFunc*) &WHFinishMD5,    NULL, 0, WHCTX_FLAG_MD5,    0, NULL, HASH_BLEN_MD5,    NULL, HASH_CLEN_MD5    + 1 };
static HASHPARMS SHA1_Parms   = { (WHInitFunc*) &WHInitSHA1,   (WHUpdateFunc*) &WHUpdateSHA1,   (WHFinishFunc*) &WHFinishSHA1,   NULL, 0, WHCTX_FLAG_SHA1,   0, NULL, HASH_BLEN_SHA1,   NULL, HASH_CLEN_SHA1   + 1 };
static HASHPARMS SHA256_Parms = { (WHInitFunc*) &WHInitSHA256, (WHUpdateFunc*) &WHUpdateSHA256, (WHFinishFunc*) &WHFinishSHA256, NULL, 0, WHCTX_FLAG_SHA256, 0, NULL, HASH_BLEN_SHA256, NULL, HASH_CLEN_SHA256 + 1 };

//-----------------------------------------------------------------------------

static DWORD WINAPI HashThread( void* pArg )
{
    HASHPARMS* p = (HASHPARMS*) pArg;

    /* Initialize... */

    if (p->flag & p->type)
        p->pInitFunc( p->pCTX );

    /* Update loop... */

    for (;;)
    {
        AcquireSRWLockShared( &g_srwLock );
        SIGNAL_DONE();
        VERIFY( SleepConditionVariableSRW( &g_cvUpdate, &g_srwLock, INFINITE, CONDITION_VARIABLE_LOCKMODE_SHARED ));
        ReleaseSRWLockShared( &g_srwLock );

        if (g_bFinish)
            break;

        if (p->flag & p->type)
            p->pUpdateFunc( p->pCTX, g_pbIn, g_cbIn );
    }

    /* Finish up... */

    if (p->flag & p->type)
    {
        p->pFinishFunc( p->pCTX );
        VERIFY( WHByteToHex( p->pBinHash,    p->cbBinHash,
                             p->pszCharHash, p->cbCharHash, p->rcase ));
    }

    SIGNAL_DONE();

    return 0;
}

//-----------------------------------------------------------------------------

static void WHSetParms( PWHCTX pContext, const BYTE* pbIn = NULL, UINT cbIn = 0, PWHRESULTS pResults = NULL, bool bFinish = false )
{
    AcquireSRWLockExclusive( &g_srwLock );
    {
        if (!pResults)
            pResults = &pContext->ctxResults;

        CRC32_Parms  . flag         =  pContext -> ctxflag;
        MD4_Parms    . flag         =  pContext -> ctxflag;
        MD5_Parms    . flag         =  pContext -> ctxflag;
        SHA1_Parms   . flag         =  pContext -> ctxflag;
        SHA256_Parms . flag         =  pContext -> ctxflag;

        CRC32_Parms  . rcase        =  pContext -> ctxCase;
        MD4_Parms    . rcase        =  pContext -> ctxCase;
        MD5_Parms    . rcase        =  pContext -> ctxCase;
        SHA1_Parms   . rcase        =  pContext -> ctxCase;
        SHA256_Parms . rcase        =  pContext -> ctxCase;

        CRC32_Parms  . pCTX         = &pContext -> ctxCRC32;
        MD4_Parms    . pCTX         = &pContext -> ctxMD4;
        MD5_Parms    . pCTX         = &pContext -> ctxMD5;
        SHA1_Parms   . pCTX         = &pContext -> ctxSHA1;
        SHA256_Parms . pCTX         = &pContext -> ctxSHA256;

        CRC32_Parms  . pBinHash     = &pContext -> ctxCRC32   . result [0];
        MD4_Parms    . pBinHash     = &pContext -> ctxMD4     . result [0];
        MD5_Parms    . pBinHash     = &pContext -> ctxMD5     . result [0];
        SHA1_Parms   . pBinHash     = &pContext -> ctxSHA1    . result [0];
        SHA256_Parms . pBinHash     = &pContext -> ctxSHA256  . result [0];

        CRC32_Parms  . pszCharHash  = &pResults -> szHexCRC32  [0];
        MD4_Parms    . pszCharHash  = &pResults -> szHexMD4    [0];
        MD5_Parms    . pszCharHash  = &pResults -> szHexMD5    [0];
        SHA1_Parms   . pszCharHash  = &pResults -> szHexSHA1   [0];
        SHA256_Parms . pszCharHash  = &pResults -> szHexSHA256 [0];

        g_pbIn    = pbIn;
        g_cbIn    = cbIn;
        g_lCtr    = 0;
        g_bFinish = bFinish;

        VERIFY( ResetEvent( g_hDone ));
    }
    ReleaseSRWLockExclusive( &g_srwLock );
}

//-----------------------------------------------------------------------------

void WHAPI WHInitEx( PWHCTX pContext )
{
    g_hDone = CreateEvent( NULL, TRUE, FALSE, NULL );

    InitializeConditionVariable ( &g_cvUpdate );
    InitializeSRWLock           ( &g_srwLock  );

    WHSetParms( pContext );  // (must do this BEFORE we create the threads)

    // (we don't need thread handle nor thread id, thus CloseHandle)

    DWORD  dwThreadId;

    VERIFY( CloseHandle( CreateThread( NULL, 0, HashThread, &CRC32_Parms,  0, &dwThreadId )));
    VERIFY( CloseHandle( CreateThread( NULL, 0, HashThread, &MD4_Parms,    0, &dwThreadId )));
    VERIFY( CloseHandle( CreateThread( NULL, 0, HashThread, &MD5_Parms,    0, &dwThreadId )));
    VERIFY( CloseHandle( CreateThread( NULL, 0, HashThread, &SHA1_Parms,   0, &dwThreadId )));
    VERIFY( CloseHandle( CreateThread( NULL, 0, HashThread, &SHA256_Parms, 0, &dwThreadId )));

    WaitForSingleObject( g_hDone, INFINITE );
}

//-----------------------------------------------------------------------------

void WHAPI WHUpdateEx( PWHCTX pContext, const BYTE* pbIn, UINT cbIn )
{
    WHSetParms( pContext, pbIn, cbIn );
    WakeAllConditionVariable( &g_cvUpdate );
    WaitForSingleObject( g_hDone, INFINITE );
}

//-----------------------------------------------------------------------------

void WHAPI WHFinishEx( PWHCTX pContext, PWHRESULTS pResults )
{
    WHSetParms( pContext, NULL, 0, pResults, true );
    WakeAllConditionVariable( &g_cvUpdate );
    WaitForSingleObject( g_hDone, INFINITE );
    VERIFY( CloseHandle( g_hDone ));
}

///////////////////////////////////////////////////////////////////////////////
//
//              WH*To* hex string conversion functions
//
///////////////////////////////////////////////////////////////////////////////

static const BYTE BinValTab[] =
{
   99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,
   99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99, 
   99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99, 
   0,1,2,3,4,5,6,7,8,9,    // 0123456789
   99,99,99,99,99,99,99,
   10,11,12,13,14,15,      // ABCDEF 
   99,99,99,99,99,99,99,99,99,99,99,99,99,
   99,99,99,99,99,99,99,99,99,99,99,99,99,
   10,11,12,13,14,15,      // abcdef 
};

//-----------------------------------------------------------------------------

BOOL WHHexToByte( LPCTSTR pszString, UINT nStrLen, PBYTE pData, UINT nDataLen )
{
   LPCTSTR psz;
   UINT nLen;

   if (0
       || (nStrLen & 1)
       || nDataLen < (nStrLen >> 1)
   )
      return FALSE;

   for (psz = pszString, nLen = nStrLen; nLen; --nLen, ++psz)
   {
      if (0
          || *psz >= _countof( BinValTab )
          || BinValTab[ *psz ] > 15
      )
         return FALSE;
   }

   for (psz = pszString, nLen = (nStrLen >> 1); nLen; --nLen, ++pData)
   {
      *pData   =  BinValTab[ *psz++ ] << 4;
      *pData  |=  BinValTab[ *psz++ ];
   }

   return TRUE;
}

//-----------------------------------------------------------------------------

static const TCHAR HexCharTab[1 << 4] =
{
   _T('0'),_T('1'),_T('2'),_T('3'),_T('4'),_T('5'),_T('6'),_T('7'),
   _T('8'),_T('9'),_T('a'),_T('b'),_T('c'),_T('d'),_T('e'),_T('f')
};

//-----------------------------------------------------------------------------

BOOL WHByteToHex( const BYTE* pData, UINT nDataLen, LPTSTR pszString, UINT nStrLen, UINT8 ctxCase )
{
   if (0
       || !pData
       || !nDataLen
       || !pszString
       || nStrLen <= (nDataLen << 1)
   )
      return FALSE;

   for (; nDataLen; --nDataLen, ++pData)
   {
      *pszString++ = HexCharTab[ *pData >> 4   ] | ctxCase;
      *pszString++ = HexCharTab[ *pData & 0x0F ] | ctxCase;
   }

   *pszString = 0;
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
