/**
 * SimpleString Library
 * Last modified: 2009/01/06
 * Copyright (C) Kai Liu.  All rights reserved.
 **/

#include "stdafx.h"
#include "SimpleString.h"

#ifdef _32_BIT

PSTR __stdcall SSChainNCpy2FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2 )
{
   __asm
   {
      // The auto-generated pushing of edi and esi onto the stack means that
      // our esp is offset by 8 bytes

      mov         edi,[esp+0x0C] // pszDest
      mov         esi,[esp+0x10] // pszSrc1
      mov         ecx,[esp+0x14] // cch1
      rep movsb
      mov         esi,[esp+0x18] // pszSrc2
      mov         ecx,[esp+0x1C] // cch2
      rep movsb
      xchg        eax,edi
   }
}

PSTR __stdcall SSChainNCpy3FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2,
                                          PCSTR pszSrc3, SIZE_T cch3 )
{
   __asm
   {
      // The auto-generated pushing of edi and esi onto the stack means that
      // our esp is offset by 8 bytes

      mov         edi,[esp+0x0C] // pszDest
      mov         esi,[esp+0x10] // pszSrc1
      mov         ecx,[esp+0x14] // cch1
      rep movsb
      mov         esi,[esp+0x18] // pszSrc2
      mov         ecx,[esp+0x1C] // cch2
      rep movsb
      mov         esi,[esp+0x20] // pszSrc3
      mov         ecx,[esp+0x24] // cch3
      rep movsb
      xchg        eax,edi
   }
}

PWSTR __stdcall SSChainNCpy2FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2 )
{
   __asm
   {
      // The auto-generated pushing of edi and esi onto the stack means that
      // our esp is offset by 8 bytes

      mov         edi,[esp+0x0C] // pszDest
      mov         esi,[esp+0x10] // pszSrc1
      mov         ecx,[esp+0x14] // cch1
      rep movsw
      mov         esi,[esp+0x18] // pszSrc2
      mov         ecx,[esp+0x1C] // cch2
      rep movsw
      xchg        eax,edi
   }
}

PWSTR __stdcall SSChainNCpy3FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2,
                                            PCWSTR pszSrc3, SIZE_T cch3 )
{
   __asm
   {
      // The auto-generated pushing of edi and esi onto the stack means that
      // our esp is offset by 8 bytes

      mov         edi,[esp+0x0C] // pszDest
      mov         esi,[esp+0x10] // pszSrc1
      mov         ecx,[esp+0x14] // cch1
      rep movsw
      mov         esi,[esp+0x18] // pszSrc2
      mov         ecx,[esp+0x1C] // cch2
      rep movsw
      mov         esi,[esp+0x20] // pszSrc3
      mov         ecx,[esp+0x24] // cch3
      rep movsw
      xchg        eax,edi
   }
}

PSTR __stdcall SSChainCpyCatA( PSTR pszDest, PCSTR pszSrc1, PCSTR pszSrc2 )
{
   __asm
   {
      // The auto-generated pushing of edi and esi onto the stack means that
      // our esp is offset by 8 bytes

      xor         eax,eax

      mov         esi,[esp+0x10] // pszSrc1
      mov         edi,esi
      or          ecx,-1
      repnz scasb
      not         ecx
      dec         ecx
      mov         edi,[esp+0x0C] // pszDest
      rep movsb

      mov         esi,[esp+0x14] // pszSrc2
      push        edi
      mov         edi,esi
      or          ecx,-1
      repnz scasb
      not         ecx
      pop         edi
      rep movsb
      dec         edi
      xchg        eax,edi
   }
}

PWSTR __stdcall SSChainCpyCatW( PWSTR pszDest, PCWSTR pszSrc1, PCWSTR pszSrc2 )
{
   __asm
   {
      // The auto-generated pushing of edi and esi onto the stack means that
      // our esp is offset by 8 bytes

      xor         eax,eax

      mov         esi,[esp+0x10] // pszSrc1
      mov         edi,esi
      or          ecx,-1
      repnz scasw
      not         ecx
      dec         ecx
      mov         edi,[esp+0x0C] // pszDest
      rep movsw

      mov         esi,[esp+0x14] // pszSrc2
      push        edi
      mov         edi,esi
      or          ecx,-1
      repnz scasw
      not         ecx
      pop         edi
      rep movsw
      dec         edi
      dec         edi
      xchg        eax,edi
   }
}

#else // _64_BIT

PSTR __stdcall SSChainNCpy2FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2 )
{
   return(SSChainNCpy2A(pszDest, pszSrc1, cch1, pszSrc2, cch2));
}

PSTR __stdcall SSChainNCpy3FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2,
                                          PCSTR pszSrc3, SIZE_T cch3 )
{
   return(SSChainNCpy3A(pszDest, pszSrc1, cch1, pszSrc2, cch2, pszSrc3, cch3));
}

PWSTR __stdcall SSChainNCpy2FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2 )
{
   return(SSChainNCpy2W(pszDest, pszSrc1, cch1, pszSrc2, cch2));
}

PWSTR __stdcall SSChainNCpy3FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2,
                                            PCWSTR pszSrc3, SIZE_T cch3 )
{
   return(SSChainNCpy3W(pszDest, pszSrc1, cch1, pszSrc2, cch2, pszSrc3, cch3));
}

PSTR __stdcall SSChainCpyCatA( PSTR pszDest, PCSTR pszSrc1, PCSTR pszSrc2 )
{
   return(SSChainNCpy2A(pszDest, pszSrc1, strlen(pszSrc1), pszSrc2, strlen(pszSrc2) + 1));
}

PWSTR __stdcall SSChainCpyCatW( PWSTR pszDest, PCWSTR pszSrc1, PCWSTR pszSrc2 )
{
   return(SSChainNCpy2W(pszDest, pszSrc1, wcslen(pszSrc1), pszSrc2, wcslen(pszSrc2) + 1));
}

#endif // _32_BIT or _64_BIT
