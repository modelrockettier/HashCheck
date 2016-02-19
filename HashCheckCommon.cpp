/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"
#include "HashCheckCommon.h"

///////////////////////////////////////////////////////////////////////////////

HANDLE __fastcall OpenFileForReading( LPCTSTR pszPath )
{
   HANDLE hFile = CreateFile
   (
      pszPath,
      GENERIC_READ,
      0
         | FILE_SHARE_READ
         | FILE_SHARE_WRITE
         | FILE_SHARE_DELETE
      ,
      NULL,
      OPEN_EXISTING,
      0
         | FILE_ATTRIBUTE_NORMAL
         | FILE_FLAG_SEQUENTIAL_SCAN
      ,
      NULL
   );
   return hFile;
}

///////////////////////////////////////////////////////////////////////////////

HANDLE __fastcall CreateWorkerThread( PBEGINTHREADEXPROC pThreadProc, void* pvParam )
{
    if (!pThreadProc)
    {
        // If we have a NULL pThreadProc, then we are starting a worker thread,
        // and there is some initialization that we need to take care of...

        pThreadProc = WorkerThreadStartup;      // (default thread func)

        THREADPARMS* pcmnctx = (THREADPARMS*) pvParam;

        pcmnctx->status       = ACTIVE;
        pcmnctx->cSentMsgs    = 0;
        pcmnctx->cHandledMsgs = 0;
        pcmnctx->hWndPBTotal  = GetDlgItem( pcmnctx->hWnd, IDC_PROG_TOTAL );
        pcmnctx->hWndPBFile   = GetDlgItem( pcmnctx->hWnd, IDC_PROG_FILE );

        SendMessage( pcmnctx->hWndPBFile, PBM_SETRANGE, 0, MAKELPARAM( 0, HM_PERCENT_FACTOR));
    }

    return (HANDLE) _beginthreadex( NULL, 0, pThreadProc, pvParam, 0, NULL );
}

///////////////////////////////////////////////////////////////////////////////

unsigned __stdcall WorkerThreadStartup( void* pArg )
{
   THREADPARMS* pcmnctx = (THREADPARMS*) pArg;

   // Exchange the THREADUNION variant member

   PFNWORKERMAIN pfnWorkerMain = pcmnctx->variant.pfnWorkerMain;
   pcmnctx->variant.pvBuffer   = VirtualAlloc( NULL, READ_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE );

   if (pcmnctx->variant.pvBuffer)
   {
      pfnWorkerMain( pcmnctx );
      VirtualFree( pcmnctx->variant.pvBuffer, 0, MEM_RELEASE );
   }

   pcmnctx->status = INACTIVE;

   if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
      PostMessage( pcmnctx->hWnd, HM_WORKERTHREAD_DONE, (WPARAM) pcmnctx, 0 );

   return 0;
}

///////////////////////////////////////////////////////////////////////////////

void WorkerThreadTogglePause( THREADPARMS* pcmnctx )
{
   if (pcmnctx->status == ACTIVE)
   {
      if (SuspendThread(pcmnctx->hThread) != -1)
      {
         pcmnctx->status = PAUSED;

         if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
         {
            SetControlText(pcmnctx->hWnd, IDC_PAUSE, IDS_HC_RESUME);
            SetProgressBarPause(pcmnctx, PBST_PAUSED);
         }
      }
   }
   else if (pcmnctx->status == PAUSED)
   {
      DWORD   dwPrevSuspendCount;
      while ((dwPrevSuspendCount = ResumeThread( pcmnctx->hThread )) != -1
          &&  dwPrevSuspendCount > 1)
      {
         continue;   // (keep resuming)
      }

      pcmnctx->status = ACTIVE;

      if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
      {
         SetControlText(pcmnctx->hWnd, IDC_PAUSE, IDS_HC_PAUSE);
         SetProgressBarPause(pcmnctx, PBST_NORMAL);
      }
   }
}

///////////////////////////////////////////////////////////////////////////////

void WorkerThreadStop( THREADPARMS* pcmnctx )
{
    if (0
        || INACTIVE          == pcmnctx->status
        || CLEANUP_COMPLETED == pcmnctx->status
    )
        return;  // (not needed or already done)

    // If the thread is paused, unpause it

    if (PAUSED == pcmnctx->status)
        WorkerThreadTogglePause( pcmnctx );

    // Signal cancellation

    VERIFY( SetEvent( pcmnctx->hCancel ));
    pcmnctx->status = CANCEL_REQUESTED;

    // Disable the control buttons

    if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
    {
        EnableWindow( GetDlgItem( pcmnctx->hWnd, IDC_PAUSE ), FALSE );
        EnableWindow( GetDlgItem( pcmnctx->hWnd, IDC_STOP  ), FALSE );
    }
}

///////////////////////////////////////////////////////////////////////////////

void WorkerThreadCleanup( THREADPARMS* pcmnctx )
{
    if (pcmnctx->status == CLEANUP_COMPLETED)
        return;  // (we already did this)

    //-------------------------------------------------------------------------
    //
    // There are only two times this function gets called:
    //
    //   Case 1:  The worker thread has exited on its own, and this function
    //            was invoked in response to the thread's exit notification.
    //
    //   Case 2:  A forced abort was requested (exit, shutdown, etc) right
    //            after calling WorkerThreadStop to ask the thread to exit.
    //
    //-------------------------------------------------------------------------

    if (pcmnctx->hThread)
    {
        if (INACTIVE != pcmnctx->status)
        {
            VERIFY( SetEvent( pcmnctx->hCancel ));
            WaitForSingleObject( pcmnctx->hThread, INFINITE );
        }

        VERIFY( CloseHandle( pcmnctx->hThread )); pcmnctx->hThread = NULL;
        VERIFY( CloseHandle( pcmnctx->hCancel )); pcmnctx->hCancel = NULL;
    }

    pcmnctx->status = CLEANUP_COMPLETED;

    // Disable controls (especially the pause/stop buttons)

    if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
    {
        static const UINT16 arCtrls[] =
        {
            IDC_STOP,
            IDC_PAUSE,
            IDC_PROG_FILE,
            IDC_PROG_TOTAL
        };

        UINT i;

        for (i=0; i < _countof( arCtrls ); ++i)
            EnableControl( pcmnctx->hWnd, arCtrls[i], FALSE );
    }
}

///////////////////////////////////////////////////////////////////////////////

void WorkerThreadHashFile( THREADPARMS* pcmnctx, LPCTSTR pszPath, PBOOL pbSuccess,
                           PWHCTX pwhctx, PWHRESULTS pwhres, FILESIZE* pFileSize )
{
    *pbSuccess = FALSE;

    // If the worker thread is working so fast that the UI cannot catch up,
    // pause for a bit to let things settle down

    while (pcmnctx->cSentMsgs > pcmnctx->cHandledMsgs + 50)
    {
        Sleep( 50 );
        if (CANCEL_REQUESTED == pcmnctx->status)
            return;
    }

    pwhctx->ctxCase = WHFMT_LOWERCASE;  // Indicate we want lower-case results
                                        // TODO: (make the above an option)

    // Use OVERLAPPED (asynchronous) sequential file access

    COvSeqFile  OvSeqFile( READ_BUFFER_SIZE, true, MAX_INFLIGHT );

    if (!(*pbSuccess = OvSeqFile.Open( pszPath )))
        return;

    // If the caller provides a way to return the file size,
    // then set the file size and send a SETSIZE notification

    DWORD64 nFileSize = OvSeqFile.FileSize();

    if (pFileSize)
    {
        pFileSize->ui64 = nFileSize;
        StrFormatKBSize(  nFileSize, pFileSize->sz, _countof( pFileSize->sz ));
        PostMessage( pcmnctx->hWnd, HM_WORKERTHREAD_SETSIZE, (WPARAM) pcmnctx, (LPARAM) pFileSize );
    }

    // Read the file and calculate the checksum, sending progress
    // updates no more frequently than once each MARQUEE_INTERVAL.

    ULONGLONG  nNewTick, nOldTick = GetTickCount64();
    UINT8      cInner = 0;

    COV* pCOV;
    bool bCancel = false;
    bool bError  = false;
    bool bIsEOF  = false;

    WHInitEx( pwhctx );

    do // Outer loop: keep going until EOF
    {
        do // Inner loop: break every 8 reads or when EOF is reached
        {
            if (OvSeqFile.Read( pCOV ) < 0)
            {
                APIFAILEDMSG( OvSeqFile.Read );
                bError = true;
            }
            else if (!(bIsEOF = OvSeqFile.IsEndOfFile()))
            {
                WHUpdateEx( pwhctx, (const BYTE*) pCOV->Buffer(), pCOV->IOBytes() );
            }
            VERIFY( pCOV->FreeCOV() );
        }
        while (!bError && !bIsEOF && (++cInner & 0x07));

        if (((nNewTick = GetTickCount64()) - nOldTick) >= MARQUEE_INTERVAL)
        {
            WPARAM nPercent;

            if (nFileSize)
                nPercent = (WPARAM) ((OvSeqFile.Offset() * HM_PERCENT_FACTOR) / nFileSize);
            else
                nPercent = (WPARAM) HM_PERCENT_FACTOR;  // (empty file; use 100%)

            PostMessage( pcmnctx->hWndPBFile, PBM_SETPOS, nPercent, 0 );
            nOldTick = nNewTick;

            bCancel =
                0
                || pcmnctx->status == CANCEL_REQUESTED
                || (WaitForSingleObject( pcmnctx->hCancel, 0 ) == WAIT_OBJECT_0)
                ;
        }
    }
    while (!bError && !bIsEOF && !bCancel);

    TRACE(_T("*** InFlightHWM = %d\n"), OvSeqFile.InFlightHWM() );

    WHFinishEx( pwhctx, pwhres );

    VERIFY( OvSeqFile.Close() );

    PostMessage( pcmnctx->hWndPBFile, PBM_SETPOS, 0, 0 );

    // Successful completion if the entire file was read (i.e. EOF reached)

    *pbSuccess = (!bError && bIsEOF && !bCancel);
}

///////////////////////////////////////////////////////////////////////////////

void __fastcall HCNormalizeString( LPTSTR psz )
{
   if (!psz) return;

   while (*psz)
   {
      switch (*psz)
      {
         case _T('\r'):
            *psz = _T('\n');
            break;

         case _T('\t'):
         case _T('"'):
         case _T('*'):
            *psz = _T(' ');
            break;

         case _T('/'):
            *psz = _T('\\');
            break;
      }

      ++psz;
   }
}

///////////////////////////////////////////////////////////////////////////////

void FormatFractionalResults( LPTSTR pszFormat, LPTSTR pszBuffer, UINT uPart, UINT uTotal )
{
   // pszFormat must be at least MAX_STRINGRES TCHARs long
   // pszBuffer must be at least MAX_STRINGMSG TCHARs long

   if (!*pszFormat)
   {
      LoadString(
         g_hModThisDll,
         (uTotal == 1) ? IDS_HP_RESULT_FMT : IDS_HP_RESULTS_FMT,
         pszFormat,
         MAX_STRINGRES
      );
   }

   if (*pszFormat != _T('!'))
      wnsprintf(pszBuffer, MAX_STRINGMSG, pszFormat, uPart, uTotal);
   else
      wnsprintf(pszBuffer, MAX_STRINGMSG, pszFormat + 1, uTotal, uPart);
}

///////////////////////////////////////////////////////////////////////////////

void SetProgressBarPause( THREADPARMS* pcmnctx, WPARAM iState )
{
   // For Windows Classic, we can change the color to indicate a pause

   COLORREF clrProgress = (iState == PBST_NORMAL) ? CLR_DEFAULT : RGB(0xFF, 0x80, 0x00);
   SendMessage(pcmnctx->hWndPBTotal, PBM_SETBARCOLOR, 0, clrProgress);
   SendMessage(pcmnctx->hWndPBFile, PBM_SETBARCOLOR, 0, clrProgress);

   // Toggle the marquee animation if applicable

   if (pcmnctx->dwFlags & HCF_MARQUEE)
      SendMessage(pcmnctx->hWndPBTotal, PBM_SETMARQUEE, iState == PBST_NORMAL, MARQUEE_INTERVAL);

  // If this is Vista, we can also set the state

  SendMessage(pcmnctx->hWndPBTotal, PBM_SETSTATE, iState, 0);
  SendMessage(pcmnctx->hWndPBFile, PBM_SETSTATE, iState, 0);

  // Vista's progress bar is buggy--if you pause it while it is animating,
  // the color will not change (but it will stop animating), so it may
  // be necessary to send another PBM_SETSTATE to get it right

  SetTimer(pcmnctx->hWnd, TIMER_ID_PAUSE, 750, NULL);
}

///////////////////////////////////////////////////////////////////////////////

__inline HANDLE GetActCtx( HMODULE hModule, LPCTSTR pszResourceName )
{
   // Wraps away the silliness of CreateActCtx, including the fact that
   // it will fail if you do not provide a valid path string even if you
   // supply a valid module handle, which is quite silly since the path is
   // just going to get translated into a module handle anyway by the API.

   ACTCTX ctx;
   TCHAR szModule[MAX_PATH << 1];

   GetModuleFileName(hModule, szModule, _countof(szModule));

   ctx.cbSize = sizeof(ctx);
   ctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
   ctx.lpSource = szModule;
   ctx.lpResourceName = pszResourceName;
   ctx.hModule = hModule;

   return(CreateActCtx(&ctx));
}

///////////////////////////////////////////////////////////////////////////////

ULONG_PTR ActivateManifest( BOOL bActivate )
{
   if (!g_bActCtxCreated)
   {
      g_bActCtxCreated = TRUE;
      g_hActCtx = GetActCtx( g_hModThisDll, ISOLATIONAWARE_MANIFEST_RESOURCE_ID );
   }

   if (g_hActCtx != INVALID_HANDLE_VALUE)
   {
      ULONG_PTR uCookie;

      if (!bActivate)
         return(1);  // Just indicate that we have a good g_hActCtx
      else if (ActivateActCtx(g_hActCtx, &uCookie))
         return(uCookie);
   }

   // We can assume that zero is never a valid cookie value...
   // * http://support.microsoft.com/kb/830033
   // * http://blogs.msdn.com/jonwis/archive/2006/01/12/512405.aspx

   return(0);
}

///////////////////////////////////////////////////////////////////////////////

void SetControlText( HWND hWnd, UINT uCtrlID, UINT uStringID )
{
   TCHAR szBuffer[MAX_STRINGMSG];
   LoadString(g_hModThisDll, uStringID, szBuffer, _countof(szBuffer));
   SetDlgItemText(hWnd, uCtrlID, szBuffer);
}

///////////////////////////////////////////////////////////////////////////////

void EnableControl( HWND hWnd, UINT uCtrlID, BOOL bEnable )
{
   HWND hWndControl = GetDlgItem(hWnd, uCtrlID);
   ShowWindow(hWndControl, (bEnable) ? SW_SHOW : SW_HIDE);
   EnableWindow(hWndControl, bEnable);
}

///////////////////////////////////////////////////////////////////////////////

ULONG_PTR __fastcall HostAddRef()
{
   LPUNKNOWN pUnknown;

   if (SHGetInstanceExplorer(&pUnknown) == S_OK)
      return((ULONG_PTR)pUnknown);
   else
      return(0);
}

///////////////////////////////////////////////////////////////////////////////

void __fastcall HostRelease( ULONG_PTR uCookie )
{
   if (uCookie)
   {
      LPUNKNOWN pUnknown = (LPUNKNOWN)uCookie;
      pUnknown->Release();
   }
}

///////////////////////////////////////////////////////////////////////////////
