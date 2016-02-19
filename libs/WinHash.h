/**
 * Windows Hashing/Checksumming Library
 * Last modified: 2009/01/13
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This is a wrapper for the MD4, MD5, SHA1 and SHA256 algorithms
 * supported natively by the Windows API. The CRC32 algorithm is
 * supported via our builtin CRC32 table driven library interface.
 **/

#pragma once

#include "crc32.h"

///////////////////////////////////////////////////////////////////////////////
/*
**          Total number of supported hash types
*/
#define NUM_HASH_TYPES          5

/*
**          Hash BYTE lengths for each supported hash
*/
#define HASH_BLEN_CRC32         4
#define HASH_BLEN_MD4           16
#define HASH_BLEN_MD5           16
#define HASH_BLEN_SHA1          20
#define HASH_BLEN_SHA256        32
/* The absolute LONGEST of any of the above  */
#define MAX_HASH_BLEN           HASH_BLEN_SHA256

/*
**          Same thing, but for hex string CHAR lengths
*/
#define HASH_CLEN_CRC32         (HASH_BLEN_CRC32  << 1)
#define HASH_CLEN_MD4           (HASH_BLEN_MD4    << 1)
#define HASH_CLEN_MD5           (HASH_BLEN_MD5    << 1)
#define HASH_CLEN_SHA1          (HASH_BLEN_SHA1   << 1)
#define HASH_CLEN_SHA256        (HASH_BLEN_SHA256 << 1)
/* The absolute LONGEST of any of the above  */
#define MAX_HASH_CLEN           (MAX_HASH_BLEN    << 1)

/*
**          Hash file extensions
*/
#define HASH_EXT_CRC32          _T(".sfv")
#define HASH_EXT_MD4            _T(".md4")
#define HASH_EXT_MD5            _T(".md5")
#define HASH_EXT_SHA1           _T(".sha1")
#define HASH_EXT_SHA256         _T(".sha2")

/*
**      Table of supported Hash file extensions
*/
extern  LPCTSTR  g_szHashExtsTab[ NUM_HASH_TYPES ] /* =
{
   HASH_EXT_CRC32,
   HASH_EXT_MD4,
   HASH_EXT_MD5,
   HASH_EXT_SHA1,
   HASH_EXT_SHA256
}*/;
/*
**                  Hash names
*/
#define HASH_NAME_CRC32         _T("CRC-32")
#define HASH_NAME_MD4           _T("MD4")
#define HASH_NAME_MD5           _T("MD5")
#define HASH_NAME_SHA1          _T("SHA-1")
#define HASH_NAME_SHA256        _T("SHA-256")

/*
**              Hash OPENFILENAME filters
*/
#define HASH_FILTER_CRC32       HASH_NAME_CRC32   _T(" (*")   \
                                HASH_EXT_CRC32    _T(")\0*")  \
                                HASH_EXT_CRC32    _T("\0")

#define HASH_FILTER_MD4         HASH_NAME_MD4     _T(" (*")   \
                                HASH_EXT_MD4      _T(")\0*")  \
                                HASH_EXT_MD4      _T("\0")

#define HASH_FILTER_MD5         HASH_NAME_MD5     _T(" (*")   \
                                HASH_EXT_MD5      _T(")\0*")  \
                                HASH_EXT_MD5      _T("\0")

#define HASH_FILTER_SHA1        HASH_NAME_SHA1    _T(" (*")   \
                                HASH_EXT_SHA1     _T(")\0*")  \
                                HASH_EXT_SHA1     _T("\0")

#define HASH_FILTER_SHA256      HASH_NAME_SHA256  _T(" (*")   \
                                HASH_EXT_SHA256   _T(")\0*")  \
                                HASH_EXT_SHA256   _T("\0")

/*
**      All OPENFILENAME filters together as one big string
*/
#define HASH_FILE_FILTERS       HASH_FILTER_CRC32       \
                                HASH_FILTER_MD4         \
                                HASH_FILTER_MD5         \
                                HASH_FILTER_SHA1        \
                                HASH_FILTER_SHA256

/*
**      Hash results printf format strings (colon aligned)
*/
#define HASH_RESULTFMT_CRC32    _T(" ")    HASH_NAME_CRC32   _T(": %s") _T(CRLF)
#define HASH_RESULTFMT_MD4      _T("    ") HASH_NAME_MD4     _T(": %s") _T(CRLF)
#define HASH_RESULTFMT_MD5      _T("    ") HASH_NAME_MD5     _T(": %s") _T(CRLF)
#define HASH_RESULTFMT_SHA1     _T("  ")   HASH_NAME_SHA1    _T(": %s") _T(CRLF)
#define HASH_RESULTFMT_SHA256   _T("")     HASH_NAME_SHA256  _T(": %s") _T(CRLF)

/*
**      All printf formats strings together as one big string
*/
#define HASH_RESULTS_FMT        _T(CRLF)                \
                                HASH_RESULTFMT_CRC32    \
                                HASH_RESULTFMT_MD4      \
                                HASH_RESULTFMT_MD5      \
                                HASH_RESULTFMT_SHA1     \
                                HASH_RESULTFMT_SHA256   \
                                _T(CRLF)
/*
**      printf buffer size needed to printf all hash results
*/
#define HASH_RESULTS_BUFSIZE    HASH_CLEN_CRC32  +     \
                                HASH_CLEN_MD4    +     \
                                HASH_CLEN_MD5    +     \
                                HASH_CLEN_SHA1   +     \
                                HASH_CLEN_SHA256 +     \
                         sizeof(HASH_RESULTS_FMT)

///////////////////////////////////////////////////////////////////////////////
//
//              Structure used by the system libraries
//
///////////////////////////////////////////////////////////////////////////////

CACHE_ALIGN
struct WINCRYPT {
   HCRYPTPROV  hCryptProv;
   HCRYPTHASH  hHash;
};
typedef struct WINCRYPT WINCRYPT, *PWINCRYPT;

///////////////////////////////////////////////////////////////////////////////
//
//          Structures used by our consistency wrapper layer
//
///////////////////////////////////////////////////////////////////////////////

CACHE_ALIGN
struct WHCTX_CRC32 {
   CRCCTX         ctx;
   unsigned long  crc32;
   BYTE           result[HASH_BLEN_CRC32];
};
typedef struct WHCTX_CRC32 WHCTX_CRC32, *PWHCTX_CRC32;

//-------------------------------------------------------

CACHE_ALIGN
struct WHCTX_MD4 {
   WINCRYPT  ctx;
   BYTE      result[HASH_BLEN_MD4];
};
typedef struct WHCTX_MD4 WHCTX_MD4, *PWHCTX_MD4;

//-------------------------------------------------------

CACHE_ALIGN
struct WHCTX_MD5 {
   WINCRYPT  ctx;
   BYTE      result[HASH_BLEN_MD5];
};
typedef struct WHCTX_MD5 WHCTX_MD5, *PWHCTX_MD5;

//-------------------------------------------------------

CACHE_ALIGN
struct WHCTX_SHA1 {
   WINCRYPT  ctx;
   BYTE      result[HASH_BLEN_SHA1];
};
typedef struct WHCTX_SHA1 WHCTX_SHA1, *PWHCTX_SHA1;

//-------------------------------------------------------

CACHE_ALIGN
struct WHCTX_SHA256 {
   WINCRYPT  ctx;
   BYTE      result[HASH_BLEN_SHA256];
};
typedef struct WHCTX_SHA256 WHCTX_SHA256, *PWHCTX_SHA256;

/*-------------------------------------------------------*\
 *  All hashes converted to character hex string format  *
\*-------------------------------------------------------*/

struct WHRESULTS {
   TCHAR szHexCRC32 [ HASH_CLEN_CRC32  + 1];
   TCHAR szHexMD4   [ HASH_CLEN_MD4    + 1];
   TCHAR szHexMD5   [ HASH_CLEN_MD5    + 1];
   TCHAR szHexSHA1  [ HASH_CLEN_SHA1   + 1];
   TCHAR szHexSHA256[ HASH_CLEN_SHA256 + 1];
};
typedef struct WHRESULTS WHRESULTS, *PWHRESULTS;

/*-------------------------------------------------------*\
 *      Hash contexts passed to the WH*Ex functions      *
\*-------------------------------------------------------*/

struct WHCTX {
   UINT8        ctxflag;        // hash type(s) we're interested in
   UINT8        ctxCase;        // WHFMT_UPPERCASE or WHFMT_LOWERCASE
   WHCTX_CRC32  ctxCRC32;       // CRC32  results context
   WHCTX_MD4    ctxMD4;         // MD4    results context
   WHCTX_MD5    ctxMD5;         // MD5    results context
   WHCTX_SHA1   ctxSHA1;        // SHA1   results context
   WHCTX_SHA256 ctxSHA256;      // SHA256 results context
   WHRESULTS    ctxResults;     // hashes in char hex string format
};
typedef struct WHCTX WHCTX, *PWHCTX;

/*-------------------------------------------------------*\
 *                    Constants                          *
\*-------------------------------------------------------*/

#define WHCTX_FLAG_CRC32        (1 << 0)
#define WHCTX_FLAG_MD4          (1 << 1)
#define WHCTX_FLAG_MD5          (1 << 2)
#define WHCTX_FLAG_SHA1         (1 << 3)
#define WHCTX_FLAG_SHA256       (1 << 4)

#define WHCTX_FLAG_ALL32        (WHCTX_FLAG_CRC32)
#define WHCTX_FLAG_ALL128       (WHCTX_FLAG_MD4 | WHCTX_FLAG_MD5)
#define WHCTX_FLAG_ALL160       (WHCTX_FLAG_SHA1)
#define WHCTX_FLAG_ALL256       (WHCTX_FLAG_SHA256)
#define WHCTX_FLAG_ALL          (WHCTX_FLAG_ALL32 | WHCTX_FLAG_ALL128 | WHCTX_FLAG_ALL160 | WHCTX_FLAG_ALL256)

#define WHFMT_UPPERCASE         0x00
#define WHFMT_LOWERCASE         0x20    // uppercase ASCII bit

///////////////////////////////////////////////////////////////////////////////
//
//              Windows Cryptography wrapper functions
//
///////////////////////////////////////////////////////////////////////////////

__inline void WinCryptInit( WINCRYPT* pCrypto, DWORD dwHashAlgorithm )
{
   // PROGRAMMING NOTE: the CRYPT_VERIFYCONTEXT flag is critical.
   // Without it, CryptAcquireContext will fail on systems which
   // have not yet created a crypto key container. Because we're
   // just calculating a hash and do not need any encryption key
   // in order to do that, we thus do not need any key container
   // making the CRYPT_VERIFYCONTEXT flag a critical requirement.
   pCrypto->hCryptProv = 0;
   pCrypto->hHash = 0;
   TRACEBOOLFUNC( CryptAcquireContext, &pCrypto->hCryptProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT );
   TRACEBOOLFUNC( CryptCreateHash, pCrypto->hCryptProv, dwHashAlgorithm, 0, 0, &pCrypto->hHash );
}

__inline void WinCryptUpdate( WINCRYPT* pCrypto, const BYTE* pbIn, UINT cbIn )
{
   if (pCrypto->hHash)
      TRACEBOOLFUNC( CryptHashData, pCrypto->hHash, pbIn, cbIn, 0 );
}

__inline void WinCryptFinish( WINCRYPT* pCrypto, PBYTE pHashBuff, UINT cbBuffSize )
{
   if (pCrypto->hHash)
   {
      DWORD hashsize = cbBuffSize;
      TRACEBOOLFUNC( CryptGetHashParam, pCrypto->hHash, HP_HASHVAL, pHashBuff, &hashsize, 0 );
      TRACEBOOLFUNC( CryptDestroyHash, pCrypto->hHash );
      pCrypto->hHash = 0;
   }
   if (pCrypto->hCryptProv)
      TRACEBOOLFUNC( CryptReleaseContext, pCrypto->hCryptProv, 0 );
   pCrypto->hCryptProv = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Wrapper layer functions to ensure a more consistent interface
//
///////////////////////////////////////////////////////////////////////////////

#define  WHAPI  __fastcall

//-----------------------------------------------------------------------------
// CRC32

__inline void WHAPI WHInitCRC32( PWHCTX_CRC32 pContext )
{
   VERIFY( crc32init( &pContext->ctx ));
}

__inline void WHAPI WHUpdateCRC32( PWHCTX_CRC32 pContext, const BYTE* pbIn, UINT cbIn )
{
   VERIFY( crc32updt( &pContext->ctx, pbIn, cbIn ));
}

__inline void WHAPI WHFinishCRC32( PWHCTX_CRC32 pContext )
{
    VERIFY( crc32done( &pContext->ctx, &pContext->crc32 ));

    pContext->result[0] = (pContext->crc32 >> 24) & 0xFF;
    pContext->result[2] = (pContext->crc32 >> 16) & 0xFF;
    pContext->result[3] = (pContext->crc32 >>  8) & 0xFF;
    pContext->result[4] = (pContext->crc32 >>  0) & 0xFF;
}

//-----------------------------------------------------------------------------
// MD4

__inline void WHAPI WHInitMD4( PWHCTX_MD4 pContext )
{
   WinCryptInit( &pContext->ctx, CALG_MD4 );
}

__inline void WHAPI WHUpdateMD4( PWHCTX_MD4 pContext, const BYTE* pbIn, UINT cbIn )
{
   WinCryptUpdate( &pContext->ctx, pbIn, cbIn );
}

__inline void WHAPI WHFinishMD4( PWHCTX_MD4 pContext )
{
   WinCryptFinish( &pContext->ctx, pContext->result, _countof( pContext->result ));
}

//-----------------------------------------------------------------------------
// MD5

__inline void WHAPI WHInitMD5( PWHCTX_MD5 pContext )
{
   WinCryptInit( &pContext->ctx, CALG_MD5 );
}

__inline void WHAPI WHUpdateMD5( PWHCTX_MD5 pContext, const BYTE* pbIn, UINT cbIn )
{
   WinCryptUpdate( &pContext->ctx, pbIn, cbIn );
}

__inline void WHAPI WHFinishMD5( PWHCTX_MD5 pContext )
{
   WinCryptFinish( &pContext->ctx, pContext->result, _countof( pContext->result ));
}

//-----------------------------------------------------------------------------
// SHA1

__inline void WHAPI WHInitSHA1( PWHCTX_SHA1 pContext )
{
   WinCryptInit( &pContext->ctx, CALG_SHA1 );
}

__inline void WHAPI WHUpdateSHA1( PWHCTX_SHA1 pContext, const BYTE* pbIn, UINT cbIn )
{
   WinCryptUpdate( &pContext->ctx, pbIn, cbIn );
}

__inline void WHAPI WHFinishSHA1( PWHCTX_SHA1 pContext )
{
   WinCryptFinish( &pContext->ctx, pContext->result, _countof( pContext->result ));
}

//-----------------------------------------------------------------------------
// SHA256

__inline void WHAPI WHInitSHA256( PWHCTX_SHA256 pContext )
{
   WinCryptInit( &pContext->ctx, CALG_SHA_256 );
}

__inline void WHAPI WHUpdateSHA256( PWHCTX_SHA256 pContext, const BYTE* pbIn, UINT cbIn )
{
   WinCryptUpdate( &pContext->ctx, pbIn, cbIn );
}

__inline void WHAPI WHFinishSHA256( PWHCTX_SHA256 pContext )
{
   WinCryptFinish( &pContext->ctx, pContext->result, _countof( pContext->result ));
}

///////////////////////////////////////////////////////////////////////////////
//
//              WH*Ex functions: These require WinHash.c
//
///////////////////////////////////////////////////////////////////////////////

void WHAPI WHInitEx( PWHCTX pContext );
void WHAPI WHUpdateEx( PWHCTX pContext, const BYTE* pbIn, UINT cbIn );
void WHAPI WHFinishEx( PWHCTX pContext, PWHRESULTS pResults );

///////////////////////////////////////////////////////////////////////////////
//
//      WH*To* hex string conversion functions: These require WinHash.c
//
///////////////////////////////////////////////////////////////////////////////

BOOL WHHexToByte( LPCTSTR pszString, UINT nStrLen, PBYTE pData, UINT nDataLen );
BOOL WHByteToHex( const BYTE* pData, UINT nDataLen, LPTSTR pszString, UINT nStrLen, UINT8 ctxCase );

///////////////////////////////////////////////////////////////////////////////
