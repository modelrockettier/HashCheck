
#include "stdafx.h"
#include "crc32.h"          // (api)
#include "crc16tab.h"       // (built by CRC32Table project)
#include "crc32tab.h"       // (built by CRC32Table project)

// --------------------------------------------------------------------

#define INIT_16         0x0000L
#define XOROT_16        0x0000L
#define MASK_16         0xFFFFL
#define TABLE_16        crc16tab
#define CAST_16         unsigned short

#define INIT_32         0xFFFFFFFFL
#define XOROT_32        0xFFFFFFFFL
#define MASK_32         0xFFFFFFFFL
#define TABLE_32        crc32tab
#define CAST_32         unsigned long

// --------------------------------------------------------------------

struct _CRCCTX
{
    unsigned long crc;
};
typedef struct _CRCCTX _CRCCTX;

// --------------------------------------------------------------------

unsigned long crc16init( CRCCTX* pctx )
{
    if (pctx)
    {
        register _CRCCTX* ctx = (_CRCCTX*) pctx;
        ctx->crc = INIT_16;
        return 1;
    }
    return 0;
}

unsigned long crc32init( CRCCTX* pctx )
{
    if (pctx)
    {
        register _CRCCTX* ctx = (_CRCCTX*) pctx;
        ctx->crc = INIT_32;
        return 1;
    }
    return 0;
}

// --------------------------------------------------------------------

unsigned long crc16updt( CRCCTX* pctx, const unsigned char* data, unsigned long len )
{
    if (pctx && data && len)
    {
        register _CRCCTX* ctx = (_CRCCTX*) pctx;
        while (len--)
            ctx->crc = (ctx->crc >> 8) ^ TABLE_16[(ctx->crc ^ *data++) & 0xFF];
        return 1;
    }
    return 0;
}

unsigned long crc32updt( CRCCTX* pctx, const unsigned char* data, unsigned long len )
{
    if (pctx && data && len)
    {
        register _CRCCTX* ctx = (_CRCCTX*) pctx;
        while (len--)
            ctx->crc = (ctx->crc >> 8) ^ TABLE_32[(ctx->crc ^ *data++) & 0xFF];
        return 1;
    }
    return 0;
}

// --------------------------------------------------------------------

unsigned long crc16done( CRCCTX* pctx, unsigned short* pcrc )
{
    if (pctx && pcrc)
    {
        register _CRCCTX* ctx = (_CRCCTX*) pctx;
        *pcrc = (CAST_16) ((ctx->crc ^ XOROT_16) & MASK_16);
        return 1;
    }
    return 0;
}

unsigned long crc32done( CRCCTX* pctx, unsigned long* pcrc )
{
    if (pctx && pcrc)
    {
        register _CRCCTX* ctx = (_CRCCTX*) pctx;
        *pcrc = (CAST_32) ((ctx->crc ^ XOROT_32) & MASK_32);
        return 1;
    }
    return 0;
}

// --------------------------------------------------------------------
