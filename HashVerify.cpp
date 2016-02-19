/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"
#include "SetAppID.h"
#include "UnicodeHelpers.h"

///////////////////////////////////////////////////////////////////////////////

#define MIN_UI_UPDATE_RATE    256   // At least every 256 messages
#define MAX_UI_UPDATE_RATE    32    // No more than every 32 messages

#define HV_COL_FILENAME       0
#define HV_COL_SIZE           1
#define HV_COL_STATUS         2
#define HV_COL_EXPECTED       3
#define HV_COL_ACTUAL         4
#define HV_COL_FIRST          HV_COL_FILENAME
#define HV_COL_LAST           HV_COL_ACTUAL
                              
#define HV_STATUS_NULL        0
#define HV_STATUS_MATCH       1
#define HV_STATUS_MISMATCH    2
#define HV_STATUS_UNREADABLE  3
#define NUM_HV_STATUS         4
                              
#define LISTVIEW_EXSTYLES     ( LVS_EX_HEADERDRAGDROP |  \
                                LVS_EX_FULLROWSELECT  |  \
                                LVS_EX_LABELTIP       |  \
                                LVS_EX_DOUBLEBUFFER )

// Fix up missing A/W aliases

#define LPNMLVDISPINFO        LPNMLVDISPINFOW
#define StrCmpLogical         StrCmpLogicalW

///////////////////////////////////////////////////////////////////////////////
// Due to the stupidity of the x64 compiler, the code emitted for the non-inline
// function is not as efficient as it is on x86

#ifdef _32_BIT
#undef  SSChainNCpy2W
#define SSChainNCpy2W SSChainNCpy2FW
#endif

///////////////////////////////////////////////////////////////////////////////

struct HVPREV
{
   UINT               idStartOfDirtyRange;
   UINT               cMatch;       // number of matches
   UINT               cMismatch;    // number of mismatches
   UINT               cUnreadable;  // number of unreadable files
};
typedef struct HVPREV HVPREV, *PHVPREV;

///////////////////////////////////////////////////////////////////////////////

struct HVSORT
{
   INT                iColumn;      // column to sort
   BOOL               bReverse;     // reverse sort?
};
typedef struct HVSORT HVSORT, *PHVSORT;

///////////////////////////////////////////////////////////////////////////////

struct HVITEM
{
   FILESIZE           filesize;
   LPTSTR             pszDisplayName;
   LPTSTR             pszExpected;
   INT16              cchDisplayName;
   UINT8              uState;
   UINT8              uStatusID;
   TCHAR              szActual[MAX_HASH_CLEN+1];
};
typedef struct HVITEM HVITEM, *PHVITEM, **PPHVITEM;

///////////////////////////////////////////////////////////////////////////////

struct HVCONTEXT
{
   THREADPARMS        parms;        // Common block (MUST be first!)

   // Members specific to HashVerify

   HWND               hWndList;     // handle of the list
   HSIMPLELIST        hList;        // where we store all the data
   PPHVITEM           index;        // index of the items in the list
   LPTSTR             pszPath;      // raw path, set by initial input
   LPTSTR             pszFileData;  // raw file data, set by initial input
   HVSORT             sort;         // sort information
   BOOL               bFreshStates; // is our copy of the item states fresh?
   UINT               cTotal;       // total number of files
   UINT               cMatch;       // number of matches
   UINT               cMismatch;    // number of mismatches
   UINT               cUnreadable;  // number of unreadable files
   HVPREV             prev;         // previous update data, used for update coalescing
   UINT               uMaxBatch;    // maximum number of UI updates to coalesce
   WHCTX              whctx;        // context for the WinHash library
   TCHAR              szStatus[NUM_HV_STATUS][MAX_STRINGRES];
};
typedef struct HVCONTEXT HVCONTEXT, *PHVCONTEXT;

/*============================================================================*\
   Function declarations
\*============================================================================*/

// Data parsing functions

__inline PBYTE HashVerifyLoadData( PHVCONTEXT phvctx );
void HashVerifyParseData( PHVCONTEXT phvctx );
BOOL ValidateHexSequence( LPTSTR psz, UINT cch );

// Worker thread

void __fastcall HashVerifyWorkerMain( void* pArg );

// Dialog general

INT_PTR CALLBACK HashVerifyDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void HashVerifyDlgInit( PHVCONTEXT phvctx );

// Dialog status

void HashVerifyUpdateSummary( PHVCONTEXT phvctx, PHVITEM pItem );

// List management

__inline void HashVerifyListInfo( PHVCONTEXT phvctx, LPNMLVDISPINFO pdi );
__inline LONG_PTR HashVerifySetColor( PHVCONTEXT phvctx, LPNMLVCUSTOMDRAW pcd );
__inline LONG_PTR HashVerifyFindItem( PHVCONTEXT phvctx, LPNMLVFINDITEM pfi );
__inline void HashVerifySortColumn( PHVCONTEXT phvctx, LPNMLISTVIEW plv );
__inline void HashVerifyReadStates( PHVCONTEXT phvctx );
__inline void HashVerifySetStates( PHVCONTEXT phvctx );
__inline INT Compare64( PLONGLONG p64A, PLONGLONG p64B );

static int __cdecl SortAscendingByFilename ( const void* ppItemA, const void* ppItemB );
static int __cdecl SortAscendingBySize     ( const void* ppItemA, const void* ppItemB );
static int __cdecl SortAscendingByStatus   ( const void* ppItemA, const void* ppItemB );
static int __cdecl SortAscendingByExpected ( const void* ppItemA, const void* ppItemB );
static int __cdecl SortAscendingByActual   ( const void* ppItemA, const void* ppItemB );

/*============================================================================*\
   Entry points / main functions
\*============================================================================*/

unsigned __stdcall HashVerifyThread( void* pArg )
{
    // Note: CHashCheck::Drop has already done the IncrementDllReference
    // for us. We just need to do DecrementDllReference when we're done.

   LPTSTR pszPath = (LPTSTR) pArg;

   // We will need to free the memory allocated for the data when done
   PBYTE pbRawData;

   // First, activate our manifest and AddRef our host
   ULONG_PTR uActCtxCookie = ActivateManifest(TRUE);
   ULONG_PTR uHostCookie = HostAddRef();

   // Allocate the context data that will persist across this session
   HVCONTEXT hvctx;

   // It's important that we zero the memory since an initial value of zero is
   // assumed for many of the elements
   memset( &hvctx, 0, sizeof( hvctx ));

   // Prep the path
   HCNormalizeString(pszPath);
   StrTrim(pszPath, _T(" "));
   hvctx.pszPath = pszPath;

   // Load the raw data
   pbRawData = HashVerifyLoadData(&hvctx);

   if (hvctx.pszFileData && (hvctx.hList = SLCreate(TRUE)))
   {
      HashVerifyParseData(&hvctx);

      DialogBoxParam(
         g_hModThisDll,
         MAKEINTRESOURCE(IDD_HASHVERF),
         NULL,
         HashVerifyDlgProc,
         (LPARAM)&hvctx
      );

      SLRelease(hvctx.hList);
   }
   else if (*pszPath)
   {
      // Technically, we could reach this point by either having a file read
      // error or a memory allocation error, but I really don't feel like
      // doing separate messages for what are supposed to be rare edge cases.
      TCHAR szFormat[MAX_STRINGRES], szMessage[0x100];
      LoadString(g_hModThisDll, IDS_HV_LOADERROR_FMT, szFormat, _countof(szFormat));
      wnsprintf(szMessage, _countof(szMessage), szFormat, pszPath);
      MessageBox(NULL, szMessage, NULL, MB_OK | MB_ICONERROR);
   }

   free(pbRawData);
   free(pszPath);

   // Clean up the manifest activation and release our host
   DeactivateManifest(uActCtxCookie);
   HostRelease(uHostCookie);

   DecrementDllReference();
   return(0);
}

///////////////////////////////////////////////////////////////////////////////

void CALLBACK HashCheck_VerifyW( HWND hWnd, HINSTANCE hInstance,
                                 PWSTR pszCmdLine, INT nCmdShow )
{
   SIZE_T cchPath = wcslen(pszCmdLine) + 1;
   LPTSTR pszPath;

   // HashVerifyThread will try to free the path passed to it since
   // it expects it to be allocated by malloc. It also expects the
   // DLL reference count to be incremented by the caller too.

   size_t nBuffSize = cchPath * sizeof(TCHAR);

   if (pszPath = (LPTSTR) malloc( nBuffSize ))
   {
      if (wcscpy_s(pszPath, nBuffSize, pszCmdLine))
      {
         IncrementDllReference();
         HashVerifyThread(pszPath);
      }
      else
      {
         free(pszPath);
      }
   }
}

/*============================================================================*\
   Data parsing functions
\*============================================================================*/

PBYTE HashVerifyLoadData( PHVCONTEXT phvctx )
{
   PBYTE pbRawData = NULL;
   HANDLE hFile;

   if ((hFile = OpenFileForReading(phvctx->pszPath)) != INVALID_HANDLE_VALUE)
   {
      LARGE_INTEGER cbRawData;
      DWORD cbBytesRead;

      if ( (GetFileSizeEx(hFile, &cbRawData)) &&
           (pbRawData = (PBYTE) malloc(cbRawData.LowPart + sizeof(DWORD))) &&
           (ReadFile(hFile, pbRawData, cbRawData.LowPart, &cbBytesRead, NULL)) &&
           (cbRawData.LowPart == cbBytesRead) )
      {
         // When we allocated a block of memory for the file data, we
         // reserved a DWORD at the end for NULL termination and to serve as
         // the extra buffer needed by IsTextUTF8...
         *((UPDWORD)(pbRawData + cbRawData.LowPart)) = 0;

         // Prepare the data for the parser...
         phvctx->pszFileData = BufferToWStr(&pbRawData, cbRawData.LowPart);
         HCNormalizeString(phvctx->pszFileData);
      }

      CloseHandle(hFile);
   }

   return(pbRawData);
}

///////////////////////////////////////////////////////////////////////////////

void HashVerifyParseData( PHVCONTEXT phvctx )
{
   LPTSTR pszData = phvctx->pszFileData;  // Points to the next line to process

   UINT cchChecksum;             // Expected length of the checksum in TCHARs
   BOOL bReverseFormat = FALSE;  // TRUE if using SFV's format of putting the checksum last
   BOOL bLinesRemaining = TRUE;  // TRUE if we have not reached the end of the data

   // Try to determine the file type from the extension
   {
      LPTSTR pszExt = StrRChr(phvctx->pszPath, NULL, _T('.'));

      if (pszExt)
      {
         if (StrCmpI(pszExt, HASH_EXT_CRC32) == 0)
         {
            phvctx->whctx.ctxflag = WHCTX_FLAG_CRC32;
            cchChecksum = HASH_CLEN_CRC32;
            bReverseFormat = TRUE;
         }
         else if (StrCmpI(pszExt, HASH_EXT_MD4) == 0)
         {
            phvctx->whctx.ctxflag = WHCTX_FLAG_MD4;
            cchChecksum = HASH_CLEN_MD4;
         }
         else if (StrCmpI(pszExt, HASH_EXT_MD5) == 0)
         {
            phvctx->whctx.ctxflag = WHCTX_FLAG_MD5;
            cchChecksum = HASH_CLEN_MD5;
         }
         else if (StrCmpI(pszExt, HASH_EXT_SHA1) == 0)
         {
            phvctx->whctx.ctxflag = WHCTX_FLAG_SHA1;
            cchChecksum = HASH_CLEN_SHA1;
         }
         else if (StrCmpI(pszExt, HASH_EXT_SHA256) == 0)
         {
            phvctx->whctx.ctxflag = WHCTX_FLAG_SHA256;
            cchChecksum = HASH_CLEN_SHA256;
         }
      }
   }

   while (bLinesRemaining)
   {
      LPTSTR pszStartOfLine;  // First non-whitespace character of the line
      LPTSTR pszEndOfLine;    // Last non-whitespace character of the line
      LPTSTR pszChecksum, pszFileName = NULL;
      INT16 cchPath;         // This INCLUDES the NULL terminator!

      // Step 1: Isolate the current line as a NULL-terminated string
      {
         pszStartOfLine = pszData;

         // Find the end of the line
         while (*pszData && *pszData != _T('\n'))
            ++pszData;

         // Terminate it if necessary, otherwise flag the end of the data
         if (*pszData)
            *pszData = 0;
         else
            bLinesRemaining = FALSE;

         pszEndOfLine = pszData;

         // Strip spaces from the end of the line...
         while (--pszEndOfLine >= pszStartOfLine && *pszEndOfLine == _T(' '))
            *pszEndOfLine = 0;

         // ...and from the start of the line
         while (*pszStartOfLine == _T(' '))
            ++pszStartOfLine;

         // Skip past this line's terminator; point at the remaining data
         ++pszData;
      }

      // Step 2a: Parse the line as SFV
      if (bReverseFormat)
      {
         pszEndOfLine -= 7;

         if (pszEndOfLine > pszStartOfLine && ValidateHexSequence(pszEndOfLine, 8))
         {
            pszChecksum = pszEndOfLine;

            // Trim spaces between the checksum and the file name
            while (--pszEndOfLine >= pszStartOfLine && *pszEndOfLine == _T(' '))
               *pszEndOfLine = 0;

            // Lines that begin with ';' are comments in SFV
            if (*pszStartOfLine && *pszStartOfLine != _T(';'))
               pszFileName = pszStartOfLine;
         }
      }

      // Step 2b: All other file formats
      else
      {
         // If we do not know the type yet, make a stab at detecting it
         if (!phvctx->whctx.ctxflag)
         {
            // PROGRAMMING NOTE: due to the way ValidateHexSequence
            // is designed we must be careful to perform our checks
            // in longest to shortest sequence.
            if (ValidateHexSequence(pszStartOfLine, 64))
            {
               cchChecksum = HASH_CLEN_SHA256;
               phvctx->whctx.ctxflag = WHCTX_FLAG_ALL256;
            }
            else if (ValidateHexSequence(pszStartOfLine, 40))
            {
               cchChecksum = HASH_CLEN_SHA1;
               phvctx->whctx.ctxflag = WHCTX_FLAG_ALL160;
            }
            else if (ValidateHexSequence(pszStartOfLine, 32))
            {
               cchChecksum = HASH_CLEN_MD5;
               phvctx->whctx.ctxflag = WHCTX_FLAG_ALL128;

               // Disambiguate from the filename, if possible
               if (StrStrI(phvctx->pszPath, _T("MD5")))
                  phvctx->whctx.ctxflag = WHCTX_FLAG_MD5;
               else if (StrStrI(phvctx->pszPath, _T("MD4")))
                  phvctx->whctx.ctxflag = WHCTX_FLAG_MD4;
            }
            else if (ValidateHexSequence(pszStartOfLine, 8))
            {
               cchChecksum = HASH_CLEN_CRC32;
               phvctx->whctx.ctxflag = WHCTX_FLAG_ALL32;
            }
         }

         // Parse the line
         if ( phvctx->whctx.ctxflag && pszEndOfLine > pszStartOfLine + cchChecksum &&
              ValidateHexSequence(pszStartOfLine, cchChecksum) )
         {
            pszChecksum = pszStartOfLine;
            pszStartOfLine += cchChecksum + 1;

            // Skip over spaces between the checksum and filename
            while (*pszStartOfLine == _T(' '))
               ++pszStartOfLine;

            if (*pszStartOfLine)
               pszFileName = pszStartOfLine;
         }
      }

      // Step 3: Do something useful with the results
      if (pszFileName && (cchPath = (INT16)(pszEndOfLine + 2 - pszFileName)) > 1)
      {
         // Since pszEndOfLine points to the character BEFORE the terminator,
         // cchLine == 1 + pszEnd - pszStart, and then +1 for the NULL
         // terminator means that we need to add 2 TCHARs to the length

         // By treating cchPath as INT16 and checking the sign, we ensure
         // that the path does not exceed 32K.

         // Create the new data block
         PHVITEM pItem = (PHVITEM) SLAddItem(phvctx->hList, NULL, sizeof(HVITEM));

         // Abort if we are out of memory
         if (!pItem) break;

         pItem->filesize.ui64 = -1;
         pItem->filesize.sz[0] = 0;
         pItem->pszDisplayName = pszFileName;
         pItem->pszExpected = pszChecksum;
         pItem->cchDisplayName = cchPath;
         pItem->uStatusID = HV_STATUS_NULL;
         pItem->szActual[0] = 0;

         ++phvctx->cTotal;

      } // If the current line was found to be valid

   } // Loop until there are no lines left

   // Build the index
   if ( phvctx->cTotal && (phvctx->index = (PPHVITEM)
        SLSetContextSize(phvctx->hList, phvctx->cTotal * sizeof(PHVITEM))) )
   {
      SLBuildIndex(phvctx->hList, (void**) phvctx->index);
   }
   else
   {
      phvctx->cTotal = 0;
   }
}

///////////////////////////////////////////////////////////////////////////////

BOOL ValidateHexSequence( LPTSTR psz, UINT cch )
{
   // Check that the given hex string matches /[0-9A-Fa-f]{cch}\b/, and if it
   // does, convert to lower-case and NULL-terminate it.

   while (cch)
   {
      TCHAR ch = *psz;

      if (ch < _T('0'))
      {
         return(FALSE);
      }
      else if (ch > _T('9'))
      {
         ch |= 0x20; // Convert to lower-case

         if (ch < _T('a') || ch > _T('f'))
            return(FALSE);

         *psz = ch;
      }

      ++psz;
      --cch;
   }

   if (!*psz || *psz == _T('\n') || *psz == _T(' '))
   {
      *psz = 0;
      return TRUE;
   }

   return(FALSE);
}

/*============================================================================*\
   Worker thread
\*============================================================================*/

void __fastcall HashVerifyWorkerMain( void* pArg )
{
   PHVCONTEXT phvctx = (PHVCONTEXT) pArg;

   // Note that ALL message communication to and from the main window MUST
   // be asynchronous, or else there may be a deadlock

   PHVITEM pItem;

   // We need to keep track of the thread's execution time so that we can do a
   // sound notification of completion when appropriate
   DWORD dwTickStart = GetTickCount();

   // Initialize the path prefix length; used for building the full path
   LPTSTR pszPathTail = StrRChr(phvctx->pszPath, NULL, _T('\\'));
   SIZE_T cchPathPrefix = (pszPathTail) ? pszPathTail + 1 - phvctx->pszPath : 0;

   while (pItem = (PHVITEM) SLGetDataAndStep(phvctx->hList))
   {
      BOOL bSuccess;

      // Part 1: Build the path
      {
         SIZE_T cchPrefix = cchPathPrefix;

         // Do not use the prefix if pszDisplayName is an absolute path
         if ( pItem->pszDisplayName[0] == _T('\\') ||
              pItem->pszDisplayName[1] == _T(':') )
         {
            cchPrefix = 0;
         }

         SSChainNCpy2W(
            phvctx->parms.variant.pszPath,
            phvctx->pszPath, cchPrefix,
            pItem->pszDisplayName, pItem->cchDisplayName
         );
      }

      // Part 2: Calculate the checksum
      WorkerThreadHashFile
      (
         &phvctx->parms,
         phvctx->parms.variant.pszPath,
         &bSuccess,
         &phvctx->whctx,
         NULL,
         &pItem->filesize
      );

      if (phvctx->parms.status == CANCEL_REQUESTED)
         return;

      // Part 3: Do something with the results
      if (bSuccess)
      {
         if (phvctx->whctx.ctxflag == WHCTX_FLAG_ALL128)
         {
            // If the MD4/MD5 STILL has not been settled by this point, then
            // settle it by a simple heuristic: if the checksum matches MD4,
            // go with that, otherwise default to MD5.
            if (StrCmpI(pItem->pszExpected, phvctx->whctx.ctxResults.szHexMD4) == 0)
               phvctx->whctx.ctxflag = WHCTX_FLAG_MD4;
            else
               phvctx->whctx.ctxflag = WHCTX_FLAG_MD5;
         }

         switch (phvctx->whctx.ctxflag)
         {
            case                                                WHCTX_FLAG_CRC32:
               SSChainNCpyW(pItem->szActual, phvctx->whctx.ctxResults.szHexCRC32,
                                     _countof(phvctx->whctx.ctxResults.szHexCRC32));  break;

            case                                                WHCTX_FLAG_MD4:
               SSChainNCpyW(pItem->szActual, phvctx->whctx.ctxResults.szHexMD4,
                                     _countof(phvctx->whctx.ctxResults.szHexMD4));    break;

            case                                                WHCTX_FLAG_MD5:
               SSChainNCpyW(pItem->szActual, phvctx->whctx.ctxResults.szHexMD5,
                                     _countof(phvctx->whctx.ctxResults.szHexMD4));    break;

            case                                                WHCTX_FLAG_SHA1:
               SSChainNCpyW(pItem->szActual, phvctx->whctx.ctxResults.szHexSHA1,
                                     _countof(phvctx->whctx.ctxResults.szHexSHA1));   break;

            case                                                WHCTX_FLAG_SHA256:
               SSChainNCpyW(pItem->szActual, phvctx->whctx.ctxResults.szHexSHA256,
                                     _countof(phvctx->whctx.ctxResults.szHexSHA256)); break;
         }

         if (StrCmpI(pItem->pszExpected, pItem->szActual) == 0)
            pItem->uStatusID = HV_STATUS_MATCH;
         else
            pItem->uStatusID = HV_STATUS_MISMATCH;
      }
      else
      {
         pItem->uStatusID = HV_STATUS_UNREADABLE;
      }

      // Part 4: Update the UI
      ++phvctx->parms.cSentMsgs;
      PostMessage(phvctx->parms.hWnd, HM_WORKERTHREAD_UPDATE, (WPARAM)phvctx, (LPARAM)pItem);
   }

   // Play a sound to signal the normal, successful termination of operations,
   // but exempt operations that were nearly instantaneous
   if (phvctx->cTotal && GetTickCount() - dwTickStart >= 2000)
      MessageBeep(MB_ICONASTERISK);
}

/*============================================================================*\
   Dialog general
\*============================================================================*/

INT_PTR CALLBACK HashVerifyDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    PHVCONTEXT phvctx;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            phvctx = (PHVCONTEXT) lParam;

            // Associate the window with the context and vice-versa

            SetAppIDForWindow( hWnd, TRUE );
            SetWindowLongPtr( hWnd, DWLP_USER, (LONG_PTR) phvctx );

            // Dialog CONTROLS must ALSO be initialized too
            // before creating worker thread

            phvctx->parms.hWnd = hWnd;     // (HashVerifyDlgInit needs it)
            HashVerifyDlgInit( phvctx );   // (initialize dialog controls)

            // Now create worker thread

            phvctx->parms.hCancel               = CreateEvent( NULL, TRUE, FALSE, NULL );
            phvctx->parms.variant.pfnWorkerMain = HashVerifyWorkerMain;
            phvctx->parms.hThread               = CreateWorkerThread( NULL, phvctx );

            if (!phvctx->parms.hThread)
                WorkerThreadCleanup( &phvctx->parms );

            // Initialize the summary

            SendMessage( phvctx->parms.hWndPBTotal, PBM_SETRANGE32, 0, phvctx->cTotal );
            HashVerifyUpdateSummary( phvctx, NULL );

            return TRUE;
        }

        case WM_DESTROY:
        {
            SetAppIDForWindow( hWnd, FALSE );
            break;
        }

        case WM_ENDSESSION:
        case WM_CLOSE:
        {
            phvctx = (PHVCONTEXT) GetWindowLongPtr( hWnd, DWLP_USER );
            goto cleanup_and_exit;
        }

        case WM_COMMAND:
        {
            phvctx = (PHVCONTEXT) GetWindowLongPtr( hWnd, DWLP_USER );

            switch (LOWORD( wParam ))
            {
                case IDC_PAUSE:
                {
                    WorkerThreadTogglePause( &phvctx->parms );
                    return TRUE;
                }

                case IDC_STOP:
                {
                    WorkerThreadStop( &phvctx->parms );
                    return TRUE;
                }

cleanup_and_exit: // IDC_EXIT
                case IDC_EXIT:
                {
                    phvctx->parms.dwFlags |= HCF_EXIT_PENDING;
                    WorkerThreadStop( &phvctx->parms );
                    WorkerThreadCleanup( &phvctx->parms );
                    EndDialog( hWnd, 0 );
                    break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR  pnm = (LPNMHDR) lParam;

            if (pnm && IDC_LIST == pnm->idFrom)
            {
                phvctx = (PHVCONTEXT) GetWindowLongPtr( hWnd, DWLP_USER );

                switch (pnm->code)
                {
                    case LVN_GETDISPINFO:
                    {
                        HashVerifyListInfo( phvctx, (LPNMLVDISPINFO) lParam );
                        return TRUE;
                    }
                    case NM_CUSTOMDRAW:
                    {
                        SetWindowLongPtr( hWnd, DWLP_MSGRESULT, HashVerifySetColor( phvctx, (LPNMLVCUSTOMDRAW) lParam ));
                        return TRUE;
                    }
                    case LVN_ODFINDITEM:
                    {
                        SetWindowLongPtr( hWnd, DWLP_MSGRESULT, HashVerifyFindItem( phvctx, (LPNMLVFINDITEM) lParam ));
                        return TRUE;
                    }
                    case LVN_COLUMNCLICK:
                    {
                        HashVerifySortColumn( phvctx, (LPNMLISTVIEW) lParam );
                        return TRUE;
                    }
                    case LVN_ITEMCHANGED:
                    {
                        if (((LPNMLISTVIEW)lParam)->uChanged & LVIF_STATE)
                            phvctx->bFreshStates = FALSE;
                        break;
                    }
                    case LVN_ODSTATECHANGED:
                    {
                        phvctx->bFreshStates = FALSE;
                        break;
                    }
                }
            }

            break;
        }

        case WM_TIMER:
        {
            // Vista: Workaround to fix their buggy progress bar

            KillTimer( hWnd, TIMER_ID_PAUSE );
            phvctx = (PHVCONTEXT) GetWindowLongPtr( hWnd, DWLP_USER );
            if (PAUSED == phvctx->parms.status)
                SetProgressBarPause( &phvctx->parms, PBST_PAUSED );
            return TRUE;
        }

        case HM_WORKERTHREAD_DONE:
        {
            phvctx = (PHVCONTEXT) wParam;
            WorkerThreadCleanup( &phvctx->parms );
            return TRUE;
        }

        case HM_WORKERTHREAD_UPDATE:
        {
            phvctx = (PHVCONTEXT) wParam;
            ++phvctx->parms.cHandledMsgs;
            HashVerifyUpdateSummary( phvctx, (PHVITEM) lParam );
            return TRUE;
        }

        case HM_WORKERTHREAD_SETSIZE:
        {
            phvctx = (PHVCONTEXT) wParam;

            // At the time we receive this message, parms.cSentMsgs will be
            // the ID of the item the worker thread is currently working on,
            // and parms.cHandledMsgs will be the ID of the item for which
            // the SETSIZE message was intended for. We will update the UI
            // ONLY if the worker thread is still working on the same item
            // at the time we receive this message. Otherwise we will just
            // wait for the UPDATE message to arrive instead.

            if (phvctx->parms.cSentMsgs == phvctx->parms.cHandledMsgs)
                ListView_RedrawItems( phvctx->hWndList, phvctx->parms.cHandledMsgs, phvctx->parms.cHandledMsgs );
            return TRUE;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

void HashVerifyDlgInit( PHVCONTEXT phvctx )
{
   HWND hWnd = phvctx->parms.hWnd;
   UINT i;

   // Load strings
   {
      static const UINT16 arStrMap[][2] =
      {
         { IDC_SUMMARY,          IDS_HV_SUMMARY    },
         { IDC_MATCH_LABEL,      IDS_HV_MATCH      },
         { IDC_MISMATCH_LABEL,   IDS_HV_MISMATCH   },
         { IDC_UNREADABLE_LABEL, IDS_HV_UNREADABLE },
         { IDC_PENDING_LABEL,    IDS_HV_PENDING    },
         { IDC_PAUSE,            IDS_HV_PAUSE      },
         { IDC_STOP,             IDS_HV_STOP       },
         { IDC_EXIT,             IDS_HV_EXIT       }
      };

      for (i = 0; i < _countof(arStrMap); ++i)
         SetControlText(hWnd, arStrMap[i][0], arStrMap[i][1]);
   }

   // Set the window icon and title
   {
      LPTSTR pszFileName = StrRChr(phvctx->pszPath, NULL, _T('\\'));

      if (!(pszFileName && *++pszFileName))
         pszFileName = phvctx->pszPath;

      SendMessage(
         hWnd,
         WM_SETTEXT,
         0,
         (LPARAM)pszFileName
      );

      SendMessage(
         hWnd,
         WM_SETICON,
         ICON_BIG, // No need to explicitly set the small icon
         (LPARAM)LoadIcon(g_hModThisDll, MAKEINTRESOURCE(IDI_FILETYPE))
      );
   }

   // Initialize the list box
   {
      LONG lHashChars;        // width of hash results in characters
      LONG dlu;               // dialog units (DLU)

      struct COLINFO {
         UINT16 iStringID;
         UINT16 iAlign;
         UINT16 iWidth;
      };
      typedef struct COLINFO COLINFO, *PCOLINFO;

#define DLUS_PER_CHAR_CX   4  // character width in dialog units

      static const COLINFO arCols[] =
      {
         // Note: widths are in DLUs (dialog units)
         { IDS_HV_COL_FILENAME, LVCFMT_LEFT,  245 },
         { IDS_HV_COL_SIZE,     LVCFMT_RIGHT,  64 },
         { IDS_HV_COL_STATUS,   LVCFMT_CENTER, 64 },
         { IDS_HV_COL_EXPECTED, LVCFMT_CENTER,  0 },
         { IDS_HV_COL_ACTUAL,   LVCFMT_CENTER,  0 },
      };

      // PROGRAMMING NOTE: purposely designed to perform
      // check in reverse (longest to shortest) sequence.
      if      (phvctx->whctx.ctxflag & WHCTX_FLAG_ALL256) lHashChars = HASH_CLEN_SHA256;
      else if (phvctx->whctx.ctxflag & WHCTX_FLAG_ALL160) lHashChars = HASH_CLEN_SHA1;
      else if (phvctx->whctx.ctxflag & WHCTX_FLAG_ALL128) lHashChars = HASH_CLEN_MD5;
      else if (phvctx->whctx.ctxflag & WHCTX_FLAG_ALL32)  lHashChars = HASH_CLEN_CRC32;

      TRACE(_T("*** lHashChars = %d\n"), lHashChars );

      // We will be using the list window handle a lot throughout HashVerify,
      // so we should cache it to reduce the number of lookups
      phvctx->hWndList = GetDlgItem(hWnd, IDC_LIST);

      // Insert the columns into the list view
      for (i = 0; i < _countof(arCols); ++i)
      {
         TCHAR szBuffer[MAX_STRINGRES];
         LVCOLUMN lvc;
         RECT rc;

         LoadString(g_hModThisDll, arCols[i].iStringID, szBuffer, _countof(szBuffer));

         if (!(dlu = arCols[i].iWidth))
         {
            // Hash results string width in DLUs,
            // with 2.5 char padding on each side.
            dlu = (lHashChars * DLUS_PER_CHAR_CX) + (5 * DLUS_PER_CHAR_CX);
            TRACE(_T("*** dlu = %d\n"), dlu );

            if (phvctx->whctx.ctxflag & WHCTX_FLAG_ALL32)
            {
               // Add extra size to accommodate the header labels
               // which are likely wider than hash results string.
               dlu += (10 * DLUS_PER_CHAR_CX);
               TRACE(_T("*** dlu = %d (adjusted)\n"), dlu );
            }
         }

         // Convert DLUs to pixels
         rc.left = dlu;
         MapDialogRect(hWnd, &rc);
         TRACE(_T("*** rc.left = %d (pixels)\n"), rc.left );

         // Add this column to list view
         lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
         lvc.fmt = arCols[i].iAlign;
         lvc.cx = rc.left; // pixels
         lvc.pszText = szBuffer;

         ListView_InsertColumn(phvctx->hWndList, i, &lvc);
      }

      // Finish initializing the list view
      ListView_SetExtendedListViewStyle(phvctx->hWndList, LISTVIEW_EXSTYLES);
      ListView_SetItemCount(phvctx->hWndList, phvctx->cTotal);

      // Use the new-fangled list view style for Vista
      SetWindowTheme(phvctx->hWndList, L"Explorer", NULL);

      phvctx->sort.iColumn = -1;
   }

   // Initialize the status strings
   {
      UINT i;

      for (i = 1; i <= 3; ++i)
      {
         LoadString(
            g_hModThisDll,
            i + (IDS_HV_STATUS_MATCH - 1),
            phvctx->szStatus[i],
            _countof(phvctx->szStatus[i])
         );
      }
   }

   // Initialize miscellaneous stuff
   {
      phvctx->uMaxBatch = (phvctx->cTotal < (MIN_UI_UPDATE_RATE * MAX_UI_UPDATE_RATE)) ?
         MAX_UI_UPDATE_RATE : phvctx->cTotal / MIN_UI_UPDATE_RATE;
   }
}

/*============================================================================*\
   Dialog status
\*============================================================================*/

void HashVerifyUpdateSummary( PHVCONTEXT phvctx, PHVITEM pItem )
{
   HWND hWnd = phvctx->parms.hWnd;
   TCHAR szFormat[MAX_STRINGRES], szBuffer[MAX_STRINGMSG];

   // If this is not the initial update and we are lagging, and our update
   // drought is not TOO long, then we should skip the update...
   BOOL bUpdateUI = !(pItem && phvctx->parms.cSentMsgs > phvctx->parms.cHandledMsgs &&
                      phvctx->parms.cHandledMsgs < phvctx->prev.idStartOfDirtyRange + phvctx->uMaxBatch);

   // Update the list
   if (pItem)
   {
      switch (pItem->uStatusID)
      {
         case HV_STATUS_MATCH:
            ++phvctx->cMatch;
            break;
         case HV_STATUS_MISMATCH:
            ++phvctx->cMismatch;
            break;
         default:
            ++phvctx->cUnreadable;
      }

      if (bUpdateUI)
      {
         ListView_RedrawItems(
            phvctx->hWndList,
            phvctx->prev.idStartOfDirtyRange,
            phvctx->parms.cHandledMsgs - 1
         );
      }
   }

   // Update the counts and progress bar
   if (bUpdateUI)
   {
      // FormatFractionalResults expects an empty format buffer on the first call
      szFormat[0] = 0;

      if (!pItem || phvctx->prev.cMatch != phvctx->cMatch)
      {
         FormatFractionalResults(szFormat, szBuffer, phvctx->cMatch, phvctx->cTotal);
         SetDlgItemText(hWnd, IDC_MATCH_RESULTS, szBuffer);
      }

      if (!pItem || phvctx->prev.cMismatch != phvctx->cMismatch)
      {
         FormatFractionalResults(szFormat, szBuffer, phvctx->cMismatch, phvctx->cTotal);
         SetDlgItemText(hWnd, IDC_MISMATCH_RESULTS, szBuffer);
      }

      if (!pItem || phvctx->prev.cUnreadable != phvctx->cUnreadable)
      {
         FormatFractionalResults(szFormat, szBuffer, phvctx->cUnreadable, phvctx->cTotal);
         SetDlgItemText(hWnd, IDC_UNREADABLE_RESULTS, szBuffer);
      }

      FormatFractionalResults(szFormat, szBuffer, phvctx->cTotal - phvctx->parms.cHandledMsgs, phvctx->cTotal);
      SetDlgItemText(hWnd, IDC_PENDING_RESULTS, szBuffer);

      SendMessage(phvctx->parms.hWndPBTotal, PBM_SETPOS, phvctx->parms.cHandledMsgs, 0);

      // Now that we've updated the UI, update the prev structure
      phvctx->prev.idStartOfDirtyRange = phvctx->parms.cHandledMsgs;
      phvctx->prev.cMatch = phvctx->cMatch;
      phvctx->prev.cMismatch = phvctx->cMismatch;
      phvctx->prev.cUnreadable = phvctx->cUnreadable;
   }

   // Update the header
   if (!(phvctx->parms.dwFlags & HVF_HAS_SET_TYPE))
   {
      LPCTSTR pszSubtitle = NULL;

      switch (phvctx->whctx.ctxflag)
      {
         case WHCTX_FLAG_CRC32:  pszSubtitle = HASH_NAME_CRC32;  break;
         case WHCTX_FLAG_MD4:    pszSubtitle = HASH_NAME_MD4;    break;
         case WHCTX_FLAG_MD5:    pszSubtitle = HASH_NAME_MD5;    break;
         case WHCTX_FLAG_SHA1:   pszSubtitle = HASH_NAME_SHA1;   break;
         case WHCTX_FLAG_SHA256: pszSubtitle = HASH_NAME_SHA256; break;
      }

      if (pszSubtitle)
      {
         LoadString(g_hModThisDll, IDS_HV_SUMMARY, szFormat, _countof(szFormat));
         wnsprintf(szBuffer, _countof(szBuffer), _T("%s (%s)"), szFormat, pszSubtitle);
         SetDlgItemText(hWnd, IDC_SUMMARY, szBuffer);
         phvctx->parms.dwFlags |= HVF_HAS_SET_TYPE;
      }
   }
}

/*============================================================================*\
   List management
\*============================================================================*/

void HashVerifyListInfo( PHVCONTEXT phvctx, LPNMLVDISPINFO pdi )
{
   if ((UINT)pdi->item.iItem >= phvctx->cTotal)
      return;  // Invalid index; by casting to unsigned, we also catch negatives

   if (pdi->item.mask & LVIF_TEXT)
   {
      PHVITEM pItem = phvctx->index[pdi->item.iItem];

      switch (pdi->item.iSubItem)
      {
         case HV_COL_FILENAME: pdi->item.pszText = pItem->pszDisplayName;              break;
         case HV_COL_SIZE:     pdi->item.pszText = pItem->filesize.sz;                 break;
         case HV_COL_STATUS:   pdi->item.pszText = phvctx->szStatus[pItem->uStatusID]; break;
         case HV_COL_EXPECTED: pdi->item.pszText = pItem->pszExpected;                 break;
         case HV_COL_ACTUAL:   pdi->item.pszText = pItem->szActual;                    break;
         default:              pdi->item.pszText = _T("");                           break;
      }
   }

   if (pdi->item.mask & LVIF_IMAGE)
      pdi->item.iImage = I_IMAGENONE;

   // We can (and should) ignore LVIF_STATE
}

///////////////////////////////////////////////////////////////////////////////

LONG_PTR HashVerifySetColor( PHVCONTEXT phvctx, LPNMLVCUSTOMDRAW pcd )
{
   switch (pcd->nmcd.dwDrawStage)
   {
      case CDDS_PREPAINT:
         return(CDRF_NOTIFYITEMDRAW);

      case CDDS_ITEMPREPAINT:
      {
         // We need to determine the highlight state during the item stage
         // because this information becomes subitem-specific if we try to
         // retrieve it when we actually need it in the subitem stage

         if (IsAppThemed())
         {
            // Clear the highlight bit...
            phvctx->parms.dwFlags &= ~HVF_ITEM_HILITE;

            // uItemState is buggy; if LVS_SHOWSELALWAYS is set, uItemState
            // will ALWAYS have the CDIS_SELECTED bit set, regardless of
            // whether the item is actually selected, so a more expensive
            // test for the LVIS_SELECTED bit is needed...
            if ( pcd->nmcd.uItemState & CDIS_HOT ||
                 ListView_GetItemState(pcd->nmcd.hdr.hwndFrom, pcd->nmcd.dwItemSpec, LVIS_SELECTED) )
            {
               phvctx->parms.dwFlags |= HVF_ITEM_HILITE;
            }
         }

         return(CDRF_NOTIFYSUBITEMDRAW);
      }

      case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
      {
         PHVITEM pItem;

         if (pcd->nmcd.dwItemSpec >= phvctx->cTotal)
            break;  // Invalid index

         pItem = phvctx->index[pcd->nmcd.dwItemSpec];

         // By default, we use the default foreground and background colors
         // except when the item is a mismatch or is unreadable, in which
         // case, we change the foreground color
         switch (pItem->uStatusID)
         {
            case HV_STATUS_MISMATCH:
               pcd->clrText = RGB(0xC0, 0x00, 0x00);
               break;

            case HV_STATUS_UNREADABLE:
               pcd->clrText = RGB(0x80, 0x80, 0x80);
               break;

            default:
               pcd->clrText = CLR_DEFAULT;
         }

         pcd->clrTextBk = CLR_DEFAULT;

         // The status column, however, deserves special treatment
         if (pcd->iSubItem == HV_COL_STATUS)
         {
            if (phvctx->parms.dwFlags & HVF_ITEM_HILITE)
            {
               // Vista-style highlighting means that the foreground
               // color can show through, but not the background color
               if (pItem->uStatusID == HV_STATUS_MATCH)
                  pcd->clrText = RGB(0x00, 0x80, 0x00);
            }
            else
            {
               switch (pItem->uStatusID)
               {
                  case HV_STATUS_MATCH:
                     pcd->clrText = RGB(0x00, 0x00, 0x00);
                     pcd->clrTextBk = RGB(0x00, 0xE0, 0x00);
                     break;

                  case HV_STATUS_MISMATCH:
                     pcd->clrText = RGB(0xFF, 0xFF, 0xFF);
                     pcd->clrTextBk = RGB(0xC0, 0x00, 0x00);
                     break;

                  case HV_STATUS_UNREADABLE:
                     pcd->clrText = RGB(0x00, 0x00, 0x00);
                     pcd->clrTextBk = RGB(0xFF, 0xE0, 0x00);
                     break;
               }
            }
         }

         break;
      }
   }

   return(CDRF_DODEFAULT);
}

///////////////////////////////////////////////////////////////////////////////

LONG_PTR HashVerifyFindItem( PHVCONTEXT phvctx, LPNMLVFINDITEM pfi )
{
   PHVITEM pItem;
   INT cchCompare, iStart = pfi->iStart;
   LONG_PTR i;

   if (pfi->lvfi.flags & (LVFI_PARAM | LVFI_NEARESTXY))
      goto not_found;  // Unsupported search types

   if (!(pfi->lvfi.flags & (LVFI_PARTIAL | LVFI_STRING)))
      goto not_found;  // No valid search type specified

   // According to the documentation, LVFI_STRING without a corresponding
   // LVFI_PARTIAL should match the FULL string, but when the user sends
   // keyboard input (which uses a partial match), the notification does not
   // have the LVFI_PARTIAL flag, so we should just always assume LVFI_PARTIAL
   // INT cchCompare = (pfi->lvfi.flags & LVFI_PARTIAL) ? 0 : 1;
   // cchCompare += wcslen(pfi->lvfi.psz);
   // The above code should have been correct, but it is not...
   cchCompare = (INT)wcslen(pfi->lvfi.psz);

   // Fix out-of-range indices; by casting to unsigned, we also catch negatives
   if ((UINT)iStart > phvctx->cTotal)
      iStart = phvctx->cTotal;

   for (i = iStart; i < (INT)phvctx->cTotal; ++i)
   {
      pItem = phvctx->index[i];
      if (StrCmpNI(pItem->pszDisplayName, pfi->lvfi.psz, cchCompare) == 0)
         return(i);
   }

   if (pfi->lvfi.flags & LVFI_WRAP)
   {
      for (i = 0; i < iStart; ++i)
      {
         pItem = phvctx->index[i];
         if (StrCmpNI(pItem->pszDisplayName, pfi->lvfi.psz, cchCompare) == 0)
            return(i);
      }
   }

   not_found: return(-1);
}

///////////////////////////////////////////////////////////////////////////////

void HashVerifySortColumn( PHVCONTEXT phvctx, LPNMLISTVIEW plv )
{
   if (phvctx->parms.status != CLEANUP_COMPLETED)
      return;  // Sorting is available only after the worker is done

   // Capture the current selection/focus state
   HashVerifyReadStates(phvctx);

   if (phvctx->sort.iColumn != plv->iSubItem)
   {
      // Change to a new column
      phvctx->sort.iColumn = plv->iSubItem;
      phvctx->sort.bReverse = FALSE;

      switch (phvctx->sort.iColumn)
      {
         case HV_COL_FILENAME:
            qsort( phvctx->index, phvctx->cTotal, sizeof(PHVITEM), SortAscendingByFilename );
            break;
         case HV_COL_SIZE:
            qsort( phvctx->index, phvctx->cTotal, sizeof(PHVITEM), SortAscendingBySize );
            break;
         case HV_COL_STATUS:
            qsort( phvctx->index, phvctx->cTotal, sizeof(PHVITEM), SortAscendingByStatus );
            break;
         case HV_COL_EXPECTED:
            qsort( phvctx->index, phvctx->cTotal, sizeof(PHVITEM), SortAscendingByExpected );
            break;
         case HV_COL_ACTUAL:
            qsort( phvctx->index, phvctx->cTotal, sizeof(PHVITEM), SortAscendingByActual );
            break;
      }
   }
   else if (phvctx->sort.bReverse)
   {
      // Clicking a column thrice in a row reverts to the original file order
      phvctx->sort.iColumn = -1;
      phvctx->sort.bReverse = FALSE;

      // We do need to validate phvctx->index to handle the edge case where
      // the list is really non-empty, but we are treating it as empty because
      // we could not allocate an index (qsort_s uses the given length while
      // SLBuildIndex uses the actual length); this is, admittedly, a very
      // extreme edge case, as it crops up only in an OOM situation where the
      // user tries to click-sort an empty list view!
      if (phvctx->index)
         SLBuildIndex(phvctx->hList, (void**) phvctx->index);
   }
   else
   {
      // Clicking a column twice in a row reverses the order; since we are
      // just reversing the order of an already-sorted column, we can just
      // naively flip the index

      if (phvctx->index)
      {
          PHVITEM   pItemTmp;
         PPHVITEM  ppItemLow   =  phvctx->index;
         PPHVITEM  ppItemHigh  =  phvctx->index + phvctx->cTotal - 1;

         while (ppItemHigh > ppItemLow)
         {
              pItemTmp   =  *ppItemLow;
            *ppItemLow   =  *ppItemHigh;
            *ppItemHigh  =    pItemTmp;

            ++ppItemLow;
            --ppItemHigh;
         }
      }

      phvctx->sort.bReverse = TRUE;
   }

   // Restore the selection/focus state
   HashVerifySetStates(phvctx);

   // Update the UI
   {
      HWND hWndHeader = ListView_GetHeader(phvctx->hWndList);
      INT i;

      HDITEM hdi;
      hdi.mask = HDI_FORMAT;

      for (i = HV_COL_FIRST; i <= HV_COL_LAST; ++i)
      {
         Header_GetItem(hWndHeader, i, &hdi);
         hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
         if (phvctx->sort.iColumn == i)
            hdi.fmt |= (phvctx->sort.bReverse) ? HDF_SORTDOWN : HDF_SORTUP;
         Header_SetItem(hWndHeader, i, &hdi);
      }

      // Invalidate all items
      ListView_RedrawItems(phvctx->hWndList, 0, phvctx->cTotal);

      // Set a light gray background on the sorted column
      ListView_SetSelectedColumn(
         phvctx->hWndList,
         (phvctx->sort.iColumn != HV_COL_STATUS) ? phvctx->sort.iColumn : -1
      );

      // Unfortunately, the list does not automatically repaint all of the
      // areas affected by SetSelectedColumn, so it is necessary to force a
      // repaint of the list view's visible areas in order to avoid artifacts
      InvalidateRect(phvctx->hWndList, NULL, FALSE);
   }
}

///////////////////////////////////////////////////////////////////////////////

void HashVerifyReadStates( PHVCONTEXT phvctx )
{
   if (!phvctx->bFreshStates)
   {
      UINT i;

      for (i = 0; i < phvctx->cTotal; ++i)
      {
         phvctx->index[i]->uState = ListView_GetItemState(
            phvctx->hWndList,
            i,
            LVIS_FOCUSED | LVIS_SELECTED
         );
      }
   }
}

///////////////////////////////////////////////////////////////////////////////

void HashVerifySetStates( PHVCONTEXT phvctx )
{
   UINT i;

   // Optimize for the case where most items are unselected
   ListView_SetItemState(phvctx->hWndList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);

   for (i = 0; i < phvctx->cTotal; ++i)
   {
      if (phvctx->index[i]->uState)
      {
         ListView_SetItemState(
            phvctx->hWndList,
            i,
            phvctx->index[i]->uState,
            LVIS_FOCUSED | LVIS_SELECTED
         );
      }
   }

   phvctx->bFreshStates = TRUE;
}

///////////////////////////////////////////////////////////////////////////////

INT Compare64( PLONGLONG p64A, PLONGLONG p64B )
{
   LARGE_INTEGER diff;
   diff.QuadPart = *p64A - *p64B;
   if (diff.HighPart)
      return(diff.HighPart);
   else
      return(diff.LowPart > 0);
}

///////////////////////////////////////////////////////////////////////////////

static INT __cdecl SortAscendingByFilename( const void* ppItemA, const void* ppItemB )
{
   register PHVITEM pItemA = *(PPHVITEM)ppItemA;
   register PHVITEM pItemB = *(PPHVITEM)ppItemB;
   return(StrCmpLogical(pItemA->pszDisplayName, pItemB->pszDisplayName));
}

static INT __cdecl SortAscendingBySize( const void* ppItemA, const void* ppItemB )
{
   register PHVITEM pItemA = *(PPHVITEM)ppItemA;
   register PHVITEM pItemB = *(PPHVITEM)ppItemB;
   return(Compare64((PLONGLONG)&pItemA->filesize.ui64, (PLONGLONG)&pItemB->filesize.ui64));
}

static INT __cdecl SortAscendingByStatus( const void* ppItemA, const void* ppItemB )
{
   register PHVITEM pItemA = *(PPHVITEM)ppItemA;
   register PHVITEM pItemB = *(PPHVITEM)ppItemB;
   return((INT8)pItemA->uStatusID - (INT8)pItemB->uStatusID);
}

static INT __cdecl SortAscendingByExpected( const void* ppItemA, const void* ppItemB )
{
   register PHVITEM pItemA = *(PPHVITEM)ppItemA;
   register PHVITEM pItemB = *(PPHVITEM)ppItemB;
   return(StrCmpI(pItemA->pszExpected, pItemB->pszExpected));
}

static INT __cdecl SortAscendingByActual( const void* ppItemA, const void* ppItemB )
{
   register PHVITEM pItemA = *(PPHVITEM)ppItemA;
   register PHVITEM pItemB = *(PPHVITEM)ppItemB;
   return(StrCmpI(pItemA->szActual, pItemB->szActual));
}

///////////////////////////////////////////////////////////////////////////////
