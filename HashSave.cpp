/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"
#include "HashCalc.h"
#include "SetAppID.h"

// (forward reference)

INT_PTR CALLBACK HashSaveDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

/*============================================================================*\
   Entry points / main functions
\*============================================================================*/

unsigned __stdcall HashSaveThread( void* pArg )
{
   HASHCONTEXT* phsctx = (HASHCONTEXT*) pArg;

   // First, activate our manifest and AddRef our host
   ULONG_PTR uActCtxCookie = ActivateManifest(TRUE);
   ULONG_PTR uHostCookie = HostAddRef();

   // Calling HashCalcPrepare with a NULL hList will cause it to calculate
   // and set cchPrefix, but it will not copy the data or walk the directories
   // (we will leave that for the worker thread to do); the reason we do a
   // limited scan now is so that we can show the file dialog (which requires
   // cchPrefix for the automatic name generation) as soon as possible
   phsctx->parms.status = INACTIVE;
   phsctx->hList = NULL;
   HashCalcPrepare(phsctx);

   // Get a file name from the user
   memset( &phsctx->ofn, 0, sizeof( phsctx->ofn ));
   HashCalcInitSave(phsctx);

   if (phsctx->hFileOut != INVALID_HANDLE_VALUE)
   {
      if (phsctx->hList = SLCreate(TRUE))
      {
         DialogBoxParam(
            g_hModThisDll,
            MAKEINTRESOURCE(IDD_HASHSAVE),
            NULL,
            HashSaveDlgProc,
            (LPARAM)phsctx
         );

         SLRelease(phsctx->hList);
      }

      CloseHandle(phsctx->hFileOut);
   }

   // This must be the last thing that we free, since this is what supports
   // our context!
   SLRelease(phsctx->hListRaw);

   // Clean up the manifest activation and release our host
   DeactivateManifest(uActCtxCookie);
   HostRelease(uHostCookie);

   DecrementDllReference();
   return(0);
}

void HashSaveStart( HWND hWndOwner, HSIMPLELIST hListRaw )
{
   // Explorer will be blocking as long as this function is running,
   // so we need to return as quickly as possible and leave the work
   // up to the thread that we are going to spawn.

   HASHCONTEXT* phsctx = (HASHCONTEXT*)
       SLSetContextSize( hListRaw, sizeof( HASHCONTEXT ));

   if (phsctx)
   {
      phsctx->parms.hWnd = hWndOwner;
      phsctx->hListRaw   = hListRaw;

      IncrementDllReference();
      SLAddRef( hListRaw );

      HANDLE hThread;

      if (hThread = CreateWorkerThread( HashSaveThread, phsctx ))
      {
          // Thread creation was successful.  The thread is
          // now responsible for decrementing the ref count.

         CloseHandle( hThread );
         return;
      }

      // Thread creation failed!

      SLRelease( hListRaw );
      DecrementDllReference();
   }
}

/*============================================================================*\
   Worker thread
\*============================================================================*/

void __fastcall HashSaveWorkerMain( void* pArg )
{
   HASHCONTEXT* phsctx = (HASHCONTEXT*) pArg;

   // Note that ALL message communication to and from the main window MUST
   // be asynchronous, or else there may be a deadlock.

   HASHITEM* pItem;

   // Prep: expand directories, max path, etc. (prefix was set by earlier call)
   PostMessage(phsctx->parms.hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM)phsctx, TRUE);
   HashCalcPrepare(phsctx);
   PostMessage(phsctx->parms.hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM)phsctx, FALSE);

   // Indicate which hash type we are after
   phsctx->whctx.ctxflag = 1 << (phsctx->ofn.nFilterIndex - 1);

   while (pItem = (HASHITEM*) SLGetDataAndStep(phsctx->hList))
   {
      // Get the hash
      WorkerThreadHashFile
      (
         &phsctx->parms,
         pItem->szPath,
         &pItem->bValid,
         &phsctx->whctx,
         &pItem->results,
         NULL
      );

      if (phsctx->parms.status == CANCEL_REQUESTED)
         return;

      // Write the data
      HashCalcWriteResult(phsctx, pItem);

      // Update the UI
      ++phsctx->parms.cSentMsgs;
      PostMessage(phsctx->parms.hWnd, HM_WORKERTHREAD_UPDATE, (WPARAM)phsctx, (LPARAM)pItem);
   }
}

/*============================================================================*\
   Dialog general
\*============================================================================*/

void HashSaveDlgInit( HASHCONTEXT* phsctx )
{
   HWND hWnd = phsctx->parms.hWnd;

   // Load strings
   {
      SetControlText(hWnd, IDC_PAUSE, IDS_HS_PAUSE);
      SetControlText(hWnd, IDC_CANCEL, IDS_HS_CANCEL);
   }

   // Set the window icon and title
   {
      LPTSTR pszFileName = phsctx->ofn.lpstrFile + phsctx->ofn.nFileOffset;
      TCHAR szFormat[MAX_STRINGRES];
      LoadString(g_hModThisDll, IDS_HS_TITLE_FMT, szFormat, _countof(szFormat));
      wnsprintf(phsctx->scratch.sz, _countof(phsctx->scratch.sz), szFormat, pszFileName);

      SendMessage(
         hWnd,
         WM_SETTEXT,
         0,
         (LPARAM)phsctx->scratch.sz
      );

      SendMessage(
         hWnd,
         WM_SETICON,
         ICON_BIG, // No need to explicitly set the small icon
         (LPARAM)LoadIcon(g_hModThisDll, MAKEINTRESOURCE(IDI_FILETYPE))
      );
   }

   // Initialize miscellaneous stuff
   {
      phsctx->parms.dwFlags = 0;
      phsctx->cTotal = 0;
   }
}

INT_PTR CALLBACK HashSaveDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HASHCONTEXT* phsctx;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            phsctx = (HASHCONTEXT*) lParam;

            // Associate the window with the context and vice-versa

            SetAppIDForWindow( hWnd, TRUE );
            SetWindowLongPtr( hWnd, DWLP_USER, (LONG_PTR) phsctx );

            // Dialog CONTROLS must ALSO be initialized too
            // before creating worker thread

            phsctx->parms.hWnd = hWnd;     // (HashSaveDlgInit needs this)
            HashSaveDlgInit( phsctx );     // (initialize dialog controls)

            // Now create worker thread

            phsctx->parms.hCancel               = CreateEvent( NULL, TRUE, FALSE, NULL );
            phsctx->parms.variant.pfnWorkerMain = HashSaveWorkerMain;
            phsctx->parms.hThread               = CreateWorkerThread( NULL, phsctx );

            if (0
                || !phsctx->parms.hThread
                || WAIT_TIMEOUT != WaitForSingleObject( phsctx->parms.hThread, 1000 )
            )
            {
                // WAIT_OBJECT_0 (it's done already) or some other serious error.
                // If it's WAIT_TIMEOUT (it's still busy), then we skip this.

                WorkerThreadCleanup( &phsctx->parms );
                EndDialog( hWnd, 0 );
            }

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
            phsctx = (HASHCONTEXT*) GetWindowLongPtr( hWnd, DWLP_USER );
            goto cleanup_and_exit;
        }

        case WM_COMMAND:
        {
            phsctx = (HASHCONTEXT*) GetWindowLongPtr( hWnd, DWLP_USER );

            switch (LOWORD( wParam ))
            {
                case IDC_PAUSE:
                {
                    WorkerThreadTogglePause( &phsctx->parms );
                    return TRUE;
                }

cleanup_and_exit: // IDC_CANCEL
                case IDC_CANCEL:
                {
                    phsctx->parms.dwFlags |= HCF_EXIT_PENDING;
                    WorkerThreadStop( &phsctx->parms );
                    WorkerThreadCleanup( &phsctx->parms );
                    EndDialog( hWnd, 0 );
                    break;
                }
            }
            break;
        }

        case WM_TIMER:
        {
            // Vista: Workaround to fix their buggy progress bar

            KillTimer( hWnd, TIMER_ID_PAUSE );
            phsctx = (HASHCONTEXT*) GetWindowLongPtr( hWnd, DWLP_USER );
            if (PAUSED == phsctx->parms.status)
                SetProgressBarPause( &phsctx->parms, PBST_PAUSED );
            return TRUE;
        }

        case HM_WORKERTHREAD_DONE:
        {
            phsctx = (HASHCONTEXT*) wParam;
            WorkerThreadCleanup( &phsctx->parms );
            EndDialog( hWnd, 0 );
            return TRUE;
        }

        case HM_WORKERTHREAD_UPDATE:
        {
            phsctx = (HASHCONTEXT*) wParam;
            ++phsctx->parms.cHandledMsgs;
            SendMessage( phsctx->parms.hWndPBTotal, PBM_SETPOS, phsctx->parms.cHandledMsgs, 0 );
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
