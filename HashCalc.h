/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

#include "HashCheckOptions.h"
#include "libs\WinHash.h"

/**
 * Much of what is in the HashCalc module used to reside within HashProp; with
 * the addition of HashSave, which shares a lot of code with HashProp (but not
 * with HashVerify), the shared portions were spun out of HashProp to form
 * HashCalc.
 *
 * This sharing was tailored to reduce code duplication (both non-compiled and
 * compiled duplication), so there will be a little bit of waste on the part
 * of HashSave (e.g., scratch.ext), but it is only a few dozen KB, and this
 * really does simplify things a lot.
 **/

// Constants
#define SCRATCH_BUFFER_SIZE     (MAX_PATH_BUFFER + HASH_RESULTS_BUFSIZE)

///////////////////////////////////////////////////////////////////////////////
// Scratch buffer

struct HASHSCRATCH
{
   union
   {
   WCHAR sz[SCRATCH_BUFFER_SIZE];
   WCHAR szW[SCRATCH_BUFFER_SIZE];
   };
   char szA[SCRATCH_BUFFER_SIZE * 3];
   BYTE ext[0x8000];  // extra padding for batching large sets of small files
};
typedef struct HASHSCRATCH HASHSCRATCH;

///////////////////////////////////////////////////////////////////////////////
// Hash creation context

struct HASHCONTEXT
{
   THREADPARMS        parms;        // Common block (MUST be first!)

   // Members specific to HashCalc

   HSIMPLELIST        hListRaw;     // data from IShellExtInit
   HSIMPLELIST        hList;        // our expanded/processed data
   HANDLE             hFileOut;     // handle of the output file
   HFONT              hFont;        // fixed-width font for the results box: handle
   WNDPROC            wpSearchBox;  // original WNDPROC for the HashProp search box
   UINT               cchMax;       // max length, in characters of the longest path (stupid SFV formatting)
   UINT               cchPrefix;    // number of characters to omit from the start of the path
   UINT               cchAdjusted;  // cchPrefix, adjusted for the path of the output file
   UINT               cTotal;       // total number of files
   UINT               cSuccess;     // total number of successfully hashed
   HASHOPTIONS        opt;          // HashCheck settings
   OPENFILENAME       ofn;          // struct used for the save dialog; this needs to persist
   TCHAR              szFormat[20]; // wnsprintf format string (either "%-123s %s\r\n" for
                                    // .sfv file format (where '123' is the tracked length of
                                    // the filename) or "%s *%s\r\n" for other file formats)
   WHCTX              whctx;        // context for the WinHash library
   UINT               obScratch;    // offset, in bytes, to the scratch, for update coalescing
   HASHSCRATCH        scratch;      // scratch buffers
};
typedef struct HASHCONTEXT HASHCONTEXT;

///////////////////////////////////////////////////////////////////////////////
// Per-file data

struct HASHITEM
{
   BOOL bValid;                     // FALSE if the file could not be opened
   UINT cchPath;                    // length of path in characters, not including NULL
   WHRESULTS results;               // hash results
   TCHAR szPath[];                  // unaltered path
};
typedef struct HASHITEM HASHITEM;

///////////////////////////////////////////////////////////////////////////////
// Public functions

void  HashCalcPrepare     ( HASHCONTEXT* phcctx );
void  HashCalcInitSave    ( HASHCONTEXT* phcctx );
BOOL  HashCalcWriteResult ( HASHCONTEXT* phcctx, HASHITEM* pItem );
void  HashCalcTogglePrep  ( HASHCONTEXT* phcctx, BOOL bState );

///////////////////////////////////////////////////////////////////////////////
