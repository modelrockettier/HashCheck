/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"
#include "HashCalc.h"

/*============================================================================*\
   Function declarations
\*============================================================================*/

// Dialog general

void HashPropDlgInit( HASHCONTEXT* phpctx );
void HashPropFitDialog( HWND hWnd );
void HashPropForceLTR( HWND hWndEdit );

// Dialog status

LRESULT CALLBACK HashPropEditProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void HashPropUpdateResults( HASHCONTEXT* phpctx, HASHITEM* pItem );
void HashPropFinalStatus( HASHCONTEXT* phpctx );

// Dialog commands

void HashPropFindText( HASHCONTEXT* phpctx, BOOL bIncremental );
void HashPropSaveResults( HASHCONTEXT* phpctx );
void HashPropOptions( HASHCONTEXT* phpctx );

/*============================================================================*\
   Entry points / main functions
\*============================================================================*/

UINT CALLBACK HashPropCallback( HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
   HSIMPLELIST hList = (HSIMPLELIST)(ppsp->lParam);

   switch (uMsg)
   {
      case PSPCB_ADDREF:
         SLAddRef(hList);
         break;

      case PSPCB_RELEASE:
         SLRelease(hList);
         break;

      case PSPCB_CREATE:
      {
         HASHCONTEXT* phpctx = (HASHCONTEXT*)
             SLSetContextSize(hList, sizeof(HASHCONTEXT));

         if (phpctx)
         {
            phpctx->parms.status = INACTIVE;
            phpctx->hListRaw = hList;
            return(1);
         }
      }
   }

   return(0);
}

/*============================================================================*\
   Worker thread
\*============================================================================*/

void __fastcall HashPropWorkerMain( void* pArg )
{
   HASHCONTEXT* phpctx = (HASHCONTEXT*) pArg;

   // Note that ALL message communication to and from the main window
   // MUST be asynchronous, or else there could easily be a deadlock.

   HASHITEM* pItem;

   // Prep: expand directories, establish prefix, etc.

   PostMessage( phpctx->parms.hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM) phpctx, TRUE );
   HashCalcPrepare( phpctx );
   PostMessage( phpctx->parms.hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM) phpctx, FALSE );

   // Indicate that we want to calculate all hash types

   phpctx->whctx.ctxflag = WHCTX_FLAG_ALL;

   // Process all files in our list...

   while (pItem = (HASHITEM*) SLGetDataAndStep( phpctx->hList ))
   {
      // Calculate the hash

      WorkerThreadHashFile
      (
         &phpctx->parms,
         pItem->szPath,
         &pItem->bValid,
         &phpctx->whctx,
         &pItem->results,
         NULL
      );

      if (phpctx->parms.status == CANCEL_REQUESTED)
         return;

      // Update the UI

      ++phpctx->parms.cSentMsgs;

      PostMessage( phpctx->parms.hWnd, HM_WORKERTHREAD_UPDATE, (WPARAM) phpctx, (LPARAM) pItem );
   }
   // end while( pItem... )
}

/*============================================================================*\
   Dialog general
\*============================================================================*/

INT_PTR CALLBACK HashPropDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HASHCONTEXT* phpctx;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            phpctx = (HASHCONTEXT*) SLGetContextData( (HSIMPLELIST) (((LPPROPSHEETPAGE)lParam)->lParam ));

            // Associate the window with the context and vice-versa

            SetWindowLongPtr( hWnd, DWLP_USER, (LONG_PTR) phpctx );

            // Dialog CONTROLS must ALSO be initialized too
            // before creating worker thread

            phpctx->parms.hWnd = hWnd;     // (HashPropDlgInit needs this)
            HashPropDlgInit( phpctx );     // (initialize dialog controls)

            // Now create worker thread

            phpctx->parms.hCancel               = CreateEvent( NULL, TRUE, FALSE, NULL );
            phpctx->parms.variant.pfnWorkerMain = HashPropWorkerMain;
            phpctx->parms.hThread               = CreateWorkerThread( NULL, phpctx );

            if (!phpctx->parms.hThread)
                WorkerThreadCleanup( &phpctx->parms );

            return TRUE;
        }

        case WM_SHOWWINDOW:
        {
            phpctx = (HASHCONTEXT*) GetWindowLongPtr( hWnd, DWLP_USER );

            if (wParam && !(phpctx->parms.dwFlags & HPF_HAS_RESIZED))
            {
                // We wait for WM_SHOWWINDOW because the reported size may not
                // be accurate during WM_INITDIALOG

                HashPropFitDialog( hWnd );
                phpctx->parms.dwFlags |= HPF_HAS_RESIZED;
            }

            HashPropForceLTR( GetDlgItem( hWnd, IDC_RESULTS ));
            break;
        }

        case WM_ENDSESSION:
        case WM_DESTROY:
        {
            phpctx = (HASHCONTEXT*) GetWindowLongPtr( hWnd, DWLP_USER );

            // Undo the search box subclassing

            SetWindowLongPtr
            (
                GetDlgItem( hWnd, IDC_SEARCHBOX ),
                GWLP_WNDPROC,
                (LONG_PTR) phpctx->wpSearchBox
            );

            // Kill the worker thread

            phpctx->parms.dwFlags |= HCF_EXIT_PENDING;
            WorkerThreadStop( &phpctx->parms );
            WorkerThreadCleanup( &phpctx->parms );

            // Cleanup

            if (phpctx->hFont) DeleteObject( phpctx->hFont );
            if (phpctx->hList) SLRelease( phpctx->hList );

            break;
        }

        case WM_COMMAND:
        {
            phpctx = (HASHCONTEXT*) GetWindowLongPtr( hWnd, DWLP_USER );

            switch (LOWORD( wParam ))
            {
                case IDC_SEARCHBOX:
                {
                    if (EN_CHANGE == HIWORD( wParam ))
                    {
                        HashPropFindText( phpctx, TRUE );
                        return TRUE;
                    }
                    break;
                }

                case IDC_FIND_NEXT:
                {
                    HashPropFindText( phpctx, FALSE );
                    return TRUE;
                }

                case IDC_PAUSE:
                {
                    WorkerThreadTogglePause( &phpctx->parms );
                    return TRUE;
                }

                case IDC_STOP:
                {
                    WorkerThreadStop( &phpctx->parms );
                    return TRUE;
                }

                case IDC_SAVE:
                {
                    HashPropSaveResults( phpctx );
                    return TRUE;
                }

                case IDC_OPTIONS:
                {
                    HashPropOptions( phpctx );
                    return TRUE;
                }

                case IDC_RESULTS:
                {
                    if (EN_ALIGN_RTL_EC == HIWORD( wParam ))
                    {
                        // Do not allow the textbox to go into RTL; unfortunately,
                        // we get this notification only if the order was changed
                        // by a keyboard shortcut, not if it was changed by the
                        // context menu -- in fact, no notification of any sort
                        // is sent when it is changed by the context menu, which
                        // seems like it may be a bug in Windows.

                        HashPropForceLTR( (HWND) lParam );
                        return TRUE;
                    }
                    break;
                }
            }
            break;
        }

        case WM_TIMER:
        {
            // Vista: Workaround to fix their buggy progress bar

            KillTimer( hWnd, TIMER_ID_PAUSE );
            phpctx = (HASHCONTEXT*) GetWindowLongPtr( hWnd, DWLP_USER );
            if (PAUSED == phpctx->parms.status)
                SetProgressBarPause( &phpctx->parms, PBST_PAUSED );
            return TRUE;
        }

        case HM_WORKERTHREAD_DONE:
        {
            phpctx = (HASHCONTEXT*) wParam;
            WorkerThreadCleanup( &phpctx->parms );
            HashPropFinalStatus( phpctx );
            return TRUE;
        }

        case HM_WORKERTHREAD_UPDATE:
        {
            phpctx = (HASHCONTEXT*) wParam;
            ++phpctx->parms.cHandledMsgs;
            HashPropUpdateResults( phpctx, (HASHITEM*) lParam );
            return TRUE;
        }

        case HM_WORKERTHREAD_TOGGLEPREP:
        {
            HashCalcTogglePrep( (HASHCONTEXT*) wParam, (BOOL) lParam );
            return TRUE;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK HashPropEditProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   HASHCONTEXT* phpctx = (HASHCONTEXT*)GetWindowLongPtr(
      (HWND)GetWindowLongPtr(hWnd, GWLP_HWNDPARENT),
      DWLP_USER
   );

   if (wParam == VK_RETURN)
   {
      if (uMsg == WM_GETDLGCODE)
      {
         return(DLGC_WANTALLKEYS);
      }
      else if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
      {
         if (uMsg == WM_KEYDOWN)
            HashPropFindText(phpctx, FALSE);

         return(0);
      }
   }

   return(CallWindowProc(phpctx->wpSearchBox, hWnd, uMsg, wParam, lParam));
}

///////////////////////////////////////////////////////////////////////////////

void HashPropDlgInit( HASHCONTEXT* phpctx )
{
   HWND hWnd = phpctx->parms.hWnd;
   UINT i;

   // Load strings
   {
      static const UINT16 arStrMap[][2] =
      {
         { IDC_STATUSBOX, IDS_HP_STATUSBOX },
         { IDC_FIND_NEXT, IDS_HP_FIND      },
         { IDC_PAUSE,     IDS_HP_PAUSE     },
         { IDC_STOP,      IDS_HP_STOP      },
         { IDC_SAVE,      IDS_HP_SAVE      },
         { IDC_OPTIONS,   IDS_HP_OPTIONS   }
      };

      for (i = 0; i < _countof(arStrMap); ++i)
         SetControlText(hWnd, arStrMap[i][0], arStrMap[i][1]);
   }

   // Initialize the results text box
   {
      // Set the font
      phpctx->opt.dwFlags = HCOF_FONT;
      OptionsLoad(&phpctx->opt);
      if (phpctx->hFont = CreateFontIndirect(&phpctx->opt.lfFont))
         SendDlgItemMessage(hWnd, IDC_RESULTS, WM_SETFONT, (WPARAM)phpctx->hFont, FALSE);

      // Eliminate the text limit
      SendDlgItemMessage(hWnd, IDC_RESULTS, EM_SETLIMITTEXT, 0, 0);
   }

   // Initialize the search text box
   {
      phpctx->wpSearchBox = (WNDPROC)SetWindowLongPtr(
         GetDlgItem(hWnd, IDC_SEARCHBOX),
         GWLP_WNDPROC,
         (LONG_PTR)HashPropEditProc
      );
   }

   // Initialize miscellaneous stuff
   {
      phpctx->hList = SLCreate(TRUE);
      phpctx->parms.dwFlags = 0;
      phpctx->cTotal = 0;
      phpctx->cSuccess = 0;
      phpctx->obScratch = 0;
      memset( &phpctx->ofn, 0, sizeof( phpctx->ofn ));
   }
}

///////////////////////////////////////////////////////////////////////////////

void HashPropFitDialog( HWND hWnd )
{
   HWND hWndResults = GetDlgItem(hWnd, IDC_RESULTS);
   RECT rcDlg, rcTop, rcBottom;
   INT dy, dx, i;

   GetClientRect(hWnd, &rcDlg);
   GetWindowRect(hWndResults, &rcTop);
   GetWindowRect(GetDlgItem(hWnd, IDC_OPTIONS), &rcBottom);

   // This is kosher because a RECT is really just two POINTs...
   ScreenToClient(hWnd, (PPOINT)&rcTop.left);
   ScreenToClient(hWnd, (PPOINT)&rcTop.right);
   ScreenToClient(hWnd, (PPOINT)&rcBottom.right);

   // Get the amount that we need to offset by...
   dy = (rcDlg.bottom - rcBottom.bottom) - (rcTop.top - rcDlg.top);
   dx = (rcDlg.right - rcTop.right) - (rcTop.left - rcDlg.left);

   if (dy > 0)
   {
      // Controls to move by dy
      static const UINT16 arCtrlsMY[] =
      {
         IDC_STATUSBOX,
         IDC_PROG_TOTAL,
         IDC_PROG_FILE,
         IDC_SEARCHBOX,
         IDC_FIND_NEXT,
         IDC_PAUSE,
         IDC_STOP,
         IDC_SAVE,
         IDC_OPTIONS
      };

      // Shift controls down by dy
      for (i = 0; i < _countof(arCtrlsMY); ++i)
      {
         HWND hWndCtrl = GetDlgItem(hWnd, arCtrlsMY[i]);
         GetWindowRect(hWndCtrl, &rcBottom);
         ScreenToClient(hWnd, (PPOINT)&rcBottom.left);
         SetWindowPos(hWndCtrl, NULL, rcBottom.left, rcBottom.top + dy,
                      0, 0, SWP_NOSIZE | SWP_NOZORDER);
      }

      // Increase the height of the results box by dy
      SetWindowPos(hWndResults, NULL, 0, 0, rcTop.right - rcTop.left,
                   dy + rcTop.bottom - rcTop.top, SWP_NOMOVE | SWP_NOZORDER);
   }

   if (dx > 0)
   {
      // Controls to resize by dx
      static const UINT16 arCtrlsSX[] =
      {
         IDC_RESULTS,
         IDC_STATUSBOX,
         IDC_PROG_TOTAL,
         IDC_PROG_FILE,
         IDC_SEARCHBOX
      };

      // Controls to move by dx
      static const UINT16 arCtrlsMX[] =
      {
         IDC_FIND_NEXT,
         IDC_PAUSE,
         IDC_STOP,
         IDC_SAVE,
         IDC_OPTIONS
      };

      // Expand controls by dx
      for (i = 0; i < _countof(arCtrlsSX); ++i)
      {
         HWND hWndCtrl = GetDlgItem(hWnd, arCtrlsSX[i]);
         GetWindowRect(hWndCtrl, &rcBottom);
         ScreenToClient(hWnd, (PPOINT)&rcBottom.left);
         ScreenToClient(hWnd, (PPOINT)&rcBottom.right);
         SetWindowPos(hWndCtrl, NULL, 0, 0, dx + rcBottom.right - rcBottom.left,
                      rcBottom.bottom - rcBottom.top, SWP_NOMOVE | SWP_NOZORDER);
      }

      // Shift controls right by dx
      for (i = 0; i < _countof(arCtrlsMX); ++i)
      {
         HWND hWndCtrl = GetDlgItem(hWnd, arCtrlsMX[i]);
         GetWindowRect(hWndCtrl, &rcBottom);
         ScreenToClient(hWnd, (PPOINT)&rcBottom.left);
         SetWindowPos(hWndCtrl, NULL, rcBottom.left + dx, rcBottom.top,
                      0, 0, SWP_NOSIZE | SWP_NOZORDER);
      }
   }
}

///////////////////////////////////////////////////////////////////////////////

void HashPropForceLTR( HWND hWndEdit )
{
   DWORD dwExStyle = (DWORD)GetWindowLongPtr(hWndEdit, GWL_EXSTYLE);
   dwExStyle &= ~(WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
   SetWindowLongPtr(hWndEdit, GWL_EXSTYLE, dwExStyle);
}

/*============================================================================*\
   Dialog status
\*============================================================================*/

void HashPropUpdateResults( HASHCONTEXT* phpctx, HASHITEM* pItem )
{
   HWND hWnd = phpctx->parms.hWnd;
   HWND hWndResults = GetDlgItem(hWnd, IDC_RESULTS);

   if (pItem->bValid)
   {
      /**
       * It turns out that when hashing large numbers of small files, the
       * worker thread can far outpace the UI thread, leaving the UI thread
       * with a large backlog of updates; the solution is to keep track of
       * the size of this backlog so that steps can be taken to alleviate it.
       *
       * 1) parms.cSentMsgs and parms.cHandledMsgs are used to keep track of the backlog;
       *    parms.cSentMsgs will always be >= to parms.cHandledMsgs (there are no race
       *    conditions to worry about), and their difference is the number
       *    of updates pending in the queue AFTER the update currently being
       *    handled is completed; therefore, zero means no pending backlog.
       * 2) If there are more than 50 backlogged updates, the worker thread
       *    will throttle back enough to keep the backlog <= 50.
       * 3) If there is any backlog at all, results will be coalesced into
       *    batches to reduce the number of costly EM_REPLACESEL calls.
       **/

      LPTSTR pszScratch;
      LPTSTR pszScratchAppend;

      // First, we can increment the success count
      ++phpctx->cSuccess;

      // Get the scratch buffer; we will be using the entire scratch struct
      // as a single monolithic buffer
      pszScratch = (LPTSTR) BYTEADD(&phpctx->scratch, phpctx->obScratch);

      // Copy the file label
      pszScratchAppend = pszScratch + LoadString(g_hModThisDll, IDS_HP_FILELABEL,
                                                 pszScratch, MAX_STRINGRES);

      // Copy the path
      pszScratchAppend = SSChainNCpyW(
         pszScratchAppend,
         pItem->szPath + phpctx->cchPrefix,
         pItem->cchPath - phpctx->cchPrefix
      );

      // Copy the results
      pszScratchAppend += wnsprintf(
         pszScratchAppend,
         HASH_RESULTS_BUFSIZE, HASH_RESULTS_FMT,
         pItem->results.szHexCRC32,
         pItem->results.szHexMD4,
         pItem->results.szHexMD5,
         pItem->results.szHexSHA1,
         pItem->results.szHexSHA256
      );

      // Update the new buffer offset for use by the next update
      phpctx->obScratch = (UINT)BYTEDIFF(pszScratchAppend, &phpctx->scratch);

      // Determine if we should flush the buffer
      if ( phpctx->parms.cSentMsgs > phpctx->parms.cHandledMsgs &&
           phpctx->obScratch + (SCRATCH_BUFFER_SIZE * sizeof(TCHAR)) <= sizeof(HASHSCRATCH) )
      {
         return;
      }

      // Flush the buffer to the text box
      phpctx->obScratch = 0;
      SendMessage(hWndResults, EM_SETSEL, -2, -2);
      SendMessage(hWndResults, EM_REPLACESEL, FALSE, (LPARAM)&phpctx->scratch);

      // ClearType will sometimes leave artifacts, so redraw if the user will
      // be looking at this text for a while
      if (phpctx->parms.cSentMsgs == phpctx->parms.cHandledMsgs)
         InvalidateRect(hWndResults, NULL, FALSE);
   }

   // Yes, this means that if we defer the text box update, we also end up
   // deferring the progress bar update too, which is what we want; progress
   // bar updates are not deferred for unreadable files, but that is an edge
   // case that we should not dwell too much on.
   SendMessage(phpctx->parms.hWndPBTotal, PBM_SETPOS, phpctx->parms.cHandledMsgs, 0);
}

///////////////////////////////////////////////////////////////////////////////

void HashPropFinalStatus( HASHCONTEXT* phpctx )
{
   TCHAR szBuffer1[MAX_STRINGRES], szBuffer2[MAX_STRINGMSG], szBuffer3[MAX_STRINGMSG];

   // FormatFractionalResults expects an empty format buffer on the first call
   szBuffer1[0] = 0;

   FormatFractionalResults(szBuffer1, szBuffer2, phpctx->cSuccess, phpctx->cTotal);

   LoadString(
      g_hModThisDll,
      IDS_HP_STATUSTEXT_FMT,
      szBuffer1,
      _countof(szBuffer1)
   );

   wnsprintf(szBuffer3, _countof(szBuffer3), szBuffer1, szBuffer2);

   SetDlgItemText(phpctx->parms.hWnd, IDC_STATUSBOX, szBuffer3);

   // Enable search controls
   EnableControl(phpctx->parms.hWnd, IDC_SEARCHBOX, TRUE);
   EnableControl(phpctx->parms.hWnd, IDC_FIND_NEXT, TRUE);

   // Enable the save button only if there are results to save
   if (phpctx->cSuccess)
      EnableControl(phpctx->parms.hWnd, IDC_SAVE, TRUE);
}

/*============================================================================*\
   Dialog commands
\*============================================================================*/

void HashPropFindText( HASHCONTEXT* phpctx, BOOL bIncremental )
{
   // Since the SimpleList context for hList is unused (the hListRaw ctx
   // is the HP ctx), we might as well make use of that as an easy fussless
   // way to allocate memory without worrying about freeing it

   HWND hWndResults = GetDlgItem(phpctx->parms.hWnd, IDC_RESULTS);
   HWND hWndSearch = GetDlgItem(phpctx->parms.hWnd, IDC_SEARCHBOX);

   SIZE_T cchNeedle = SendMessage(hWndSearch, WM_GETTEXTLENGTH, 0, 0);
   LPTSTR pszNeedle = (LPTSTR) SLSetContextSize(phpctx->hList, ((UINT)cchNeedle + 1) * sizeof(TCHAR));
   LPTSTR pszHaystack;
   LPTSTR pszFound = NULL;

   DWORD dwPos;

   if (bIncremental)
      SendMessage(hWndResults, EM_GETSEL, (WPARAM)&dwPos, (LPARAM)NULL);
   else
      SendMessage(hWndResults, EM_GETSEL, (WPARAM)NULL, (LPARAM)&dwPos);

   if (pszNeedle && SendMessage(hWndSearch, WM_GETTEXT, cchNeedle + 1, (LPARAM)pszNeedle))
   {
      HLOCAL hResults = (HLOCAL)SendMessage(hWndResults, EM_GETHANDLE, 0, 0);

      // Dangling whitespace should not affect search results
      StrTrim(pszNeedle, _T(" \t\r\n"));
      cchNeedle = wcslen(pszNeedle);

      if (cchNeedle && hResults && (pszHaystack = (LPTSTR) LocalLock(hResults)))
      {
         pszFound = StrStrI(pszHaystack + dwPos, pszNeedle);
         if (!pszFound) pszFound = StrStrI(pszHaystack, pszNeedle);

         if (pszFound)
         {
            dwPos = (DWORD)(pszFound - pszHaystack);
            SendMessage(hWndResults, EM_SETSEL, dwPos, dwPos + cchNeedle);
            SendMessage(hWndResults, EM_SCROLLCARET, 0, 0);
         }

         LocalUnlock(hResults);
      }
   }

   if (!cchNeedle && bIncremental)
   {
      SendMessage(hWndResults, EM_SETSEL, dwPos, dwPos);
   }
   else if (!pszFound)
   {
      TCHAR szBuffer[MAX_STRINGMSG];

      EDITBALLOONTIP ebt;
      ebt.cbStruct = sizeof(ebt);
      ebt.pszTitle = NULL;
      ebt.pszText = szBuffer;
      ebt.ttiIcon = TTI_NONE;

      LoadString(
         g_hModThisDll,
         (cchNeedle) ? IDS_HP_FIND_NOTFOUND : IDS_HP_FIND_NOSTRING,
         szBuffer,
         _countof(szBuffer)
      );

      SendMessage(hWndSearch, EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
   }
}

///////////////////////////////////////////////////////////////////////////////

void HashPropSaveResults( HASHCONTEXT* phpctx )
{
   // HashCalcInitSave will set the file handle and output format
   HashCalcInitSave(phpctx);

   if (phpctx->hFileOut != INVALID_HANDLE_VALUE)
   {
      HASHITEM* pItem;

      SLReset(phpctx->hList);

      while (pItem = (HASHITEM*) SLGetDataAndStep(phpctx->hList))
         HashCalcWriteResult(phpctx, pItem);

      CloseHandle(phpctx->hFileOut);
   }
}

///////////////////////////////////////////////////////////////////////////////

void HashPropOptions( HASHCONTEXT* phpctx )
{
   OptionsDialog(phpctx->parms.hWnd, &phpctx->opt);

   if (phpctx->opt.dwFlags & HCOF_FONT)
   {
      HFONT hFont = CreateFontIndirect(&phpctx->opt.lfFont);

      if (hFont)
      {
         SendDlgItemMessage(phpctx->parms.hWnd, IDC_RESULTS, WM_SETFONT, (WPARAM)hFont, TRUE);
         if (phpctx->hFont) DeleteObject(phpctx->hFont);
         phpctx->hFont = hFont;
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
