/**
 * Endianness Conversion (Swap) Library
 * Last modified: 2008/12/13
 *
 * Provides an efficient way to reverse the endianness of words or dwords, if
 * the compiler does not provide an intrinsic for it, and also provides macros
 * for use with constants that can be computed at compile-time.
 **/

#pragma once

// -----------------------------------------------------------------------------
// SwapA16/SwapA32: swpay an array of 16-bit WORDs or 32-bit DWORDs...

__inline void SwapA16( unsigned short *data, size_t words )
{
   while (words)
   {
      *data = _byteswap_ushort(*data);
      ++data;
      --words;
   }
}
