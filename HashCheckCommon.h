/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

#include "HashCheckUI.h"
#include "libs\WinHash.h"
#include "COvSeqFile.h"

///////////////////////////////////////////////////////////////////////////////
// File size

struct FILESIZE
{
    ULONGLONG  ui64;        // uint64 representation
    TCHAR      sz[32];      // string representation
};
typedef struct FILESIZE FILESIZE;

///////////////////////////////////////////////////////////////////////////////
// Worker thread status

volatile enum THREADSTATUS
{
    INACTIVE = 0,
    ACTIVE,
    PAUSED,
    CANCEL_REQUESTED,
    CLEANUP_COMPLETED
};
typedef enum THREADSTATUS THREADSTATUS;

///////////////////////////////////////////////////////////////////////////////
// Extra worker thread parameters: start function address and read buffer

union THREADUNION                   // union: one value -OR- another
{
    PFNWORKERMAIN  pfnWorkerMain;   //        ptr to:  thread function,
    void*          pvBuffer;        //  -OR-  ptr to:  i/o buffer,
    LPTSTR         pszPath;         //  -OR-  ptr to:  filename.
};
typedef union THREADUNION THREADUNION;

///////////////////////////////////////////////////////////////////////////////
// Worker thread context  --  ALL OTHER CONTEXTS MUST START WITH THIS!

struct THREADPARMS
{
    THREADSTATUS  status;         // thread status
    DWORD         dwFlags;        // misc. status flags
    MSGCOUNT      cSentMsgs;      // number update messages sent by the worker
    MSGCOUNT      cHandledMsgs;   // number update messages processed by the UI
    HWND          hWnd;           // handle of the dialog window
    HWND          hWndPBTotal;    // cache of the IDC_PROG_TOTAL progress bar handle
    HWND          hWndPBFile;     // cache of the IDC_PROG_FILE progress bar handle
    HANDLE        hThread;        // handle of the worker thread
    HANDLE        hCancel;        // handle of thread cancel event
    THREADUNION   variant;        // extra parameter with varying uses
};
typedef struct THREADPARMS THREADPARMS;

///////////////////////////////////////////////////////////////////////////////
// Worker thread functions

HANDLE   __fastcall CreateWorkerThread  ( PBEGINTHREADEXPROC pThreadProc, void* pvParam );
unsigned __stdcall  WorkerThreadStartup ( void* pArg );

void  WorkerThreadTogglePause ( THREADPARMS*  pcmnctx );
void  WorkerThreadStop        ( THREADPARMS*  pcmnctx );
void  WorkerThreadCleanup     ( THREADPARMS*  pcmnctx );
void  WorkerThreadHashFile    ( THREADPARMS*  pcmnctx,
                                LPCTSTR       pszPath,
                                PBOOL         pbSuccess,
                                PWHCTX        pwhctx,
                                PWHRESULTS    pwhres,
                                FILESIZE*     pFileSize
                              );

HANDLE __fastcall OpenFileForReading( LPCTSTR pszPath );

///////////////////////////////////////////////////////////////////////////////
// Wrappers for SHGetInstanceExplorer

ULONG_PTR __fastcall HostAddRef();
void      __fastcall HostRelease( ULONG_PTR uCookie );

///////////////////////////////////////////////////////////////////////////////
// UI-related functions

void  EnableControl(  HWND hWnd, UINT uCtrlID, BOOL bEnable );
void  SetControlText( HWND hWnd, UINT uCtrlID, UINT uStringID );
void  FormatFractionalResults( LPTSTR pszFormat, LPTSTR pszBuffer, UINT uPart, UINT uTotal );
void  SetProgressBarPause( THREADPARMS* pcmnctx, WPARAM iState );

///////////////////////////////////////////////////////////////////////////////
// Parsing helpers

void __fastcall HCNormalizeString( LPTSTR psz );

///////////////////////////////////////////////////////////////////////////////
