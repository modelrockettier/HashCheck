/**
 * SimpleString Library
 * Last modified: 2009/01/06
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This is a custom C string library that provides wide-character inline
 * intrinsics for older compilers as well as some helpful chained functions.
 **/

#pragma once



#define CAST2WORD(a)                ((WORD)((DWORD_PTR)(a) & 0xFFFF))

#define WCHARS2DWORD(a, b)          ((DWORD)(CAST2WORD(a) | ((DWORD)CAST2WORD(b)) << 16))
#define SSCpy2ChW(s, a, b)          PUTDWORD(s, WCHARS2DWORD(a, b))




















__inline PSTR SSChainNCpyA( PSTR pszDest, PCSTR pszSrc, SIZE_T cch )
{
   __movsb((unsigned char *)pszDest, (unsigned char const *)pszSrc, cch);
   return(pszDest + cch);
}

__inline PWSTR SSChainNCpyW( PWSTR pszDest, PCWSTR pszSrc, SIZE_T cch )
{
   __movsw((unsigned short *)pszDest, (unsigned short const *)pszSrc, cch);
   return(pszDest + cch);
}













#if defined(_32_BIT)

__inline PSTR SSChainNCpy2A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2 )
{
   __asm
   {
      mov         edi,pszDest
      mov         esi,pszSrc1
      mov         ecx,cch1
      rep movsb
      mov         esi,pszSrc2
      mov         ecx,cch2
      rep movsb
      xchg        eax,edi
   }
}

__inline PSTR SSChainNCpy3A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2,
                                           PCSTR pszSrc3, SIZE_T cch3 )
{
   __asm
   {
      mov         edi,pszDest
      mov         esi,pszSrc1
      mov         ecx,cch1
      rep movsb
      mov         esi,pszSrc2
      mov         ecx,cch2
      rep movsb
      mov         esi,pszSrc3
      mov         ecx,cch3
      rep movsb
      xchg        eax,edi
   }
}

__inline PWSTR SSChainNCpy2W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2 )
{
   __asm
   {
      mov         edi,pszDest
      mov         esi,pszSrc1
      mov         ecx,cch1
      rep movsw
      mov         esi,pszSrc2
      mov         ecx,cch2
      rep movsw
      xchg        eax,edi
   }
}

__inline PWSTR SSChainNCpy3W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2,
                                             PCWSTR pszSrc3, SIZE_T cch3 )
{
   __asm
   {
      mov         edi,pszDest
      mov         esi,pszSrc1
      mov         ecx,cch1
      rep movsw
      mov         esi,pszSrc2
      mov         ecx,cch2
      rep movsw
      mov         esi,pszSrc3
      mov         ecx,cch3
      rep movsw
      xchg        eax,edi
   }
}

#else

__inline PSTR SSChainNCpy2A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2 )
{
   pszDest = SSChainNCpyA(pszDest, pszSrc1, cch1);
   return(SSChainNCpyA(pszDest, pszSrc2, cch2));
}

__inline PSTR SSChainNCpy3A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2,
                                           PCSTR pszSrc3, SIZE_T cch3 )
{
   pszDest = SSChainNCpyA(pszDest, pszSrc1, cch1);
   pszDest = SSChainNCpyA(pszDest, pszSrc2, cch2);
   return(SSChainNCpyA(pszDest, pszSrc3, cch3));
}

__inline PWSTR SSChainNCpy2W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2 )
{
   pszDest = SSChainNCpyW(pszDest, pszSrc1, cch1);
   return(SSChainNCpyW(pszDest, pszSrc2, cch2));
}

__inline PWSTR SSChainNCpy3W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2,
                                             PCWSTR pszSrc3, SIZE_T cch3 )
{
   pszDest = SSChainNCpyW(pszDest, pszSrc1, cch1);
   pszDest = SSChainNCpyW(pszDest, pszSrc2, cch2);
   return(SSChainNCpyW(pszDest, pszSrc3, cch3));
}

#endif





PSTR  __stdcall SSChainNCpy2FA( PSTR  pszDest, PCSTR  pszSrc1, SIZE_T cch1,
                                            PCSTR  pszSrc2, SIZE_T cch2 );

PSTR  __stdcall SSChainNCpy3FA( PSTR  pszDest, PCSTR  pszSrc1, SIZE_T cch1,
                                            PCSTR  pszSrc2, SIZE_T cch2,
                                            PCSTR  pszSrc3, SIZE_T cch3 );

PWSTR __stdcall SSChainNCpy2FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2 );

PWSTR __stdcall SSChainNCpy3FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2,
                                            PCWSTR pszSrc3, SIZE_T cch3 );

PSTR  __stdcall SSChainCpyCatA( PSTR pszDest, PCSTR pszSrc1, PCSTR pszSrc2 );

PWSTR __stdcall SSChainCpyCatW( PWSTR pszDest, PCWSTR pszSrc1, PCWSTR pszSrc2 );

