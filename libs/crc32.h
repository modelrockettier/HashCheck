
#pragma once

typedef void* CRCCTX;

extern  unsigned long crc16init( CRCCTX* pctx );
extern  unsigned long crc16updt( CRCCTX* pctx, const unsigned char* data, unsigned long len );
extern  unsigned long crc16done( CRCCTX* pctx, unsigned short* pcrc );

extern  unsigned long crc32init( CRCCTX* pctx );
extern  unsigned long crc32updt( CRCCTX* pctx, const unsigned char* data, unsigned long len );
extern  unsigned long crc32done( CRCCTX* pctx, unsigned long* pcrc );
