/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

unsigned __stdcall  HashVerifyThread ( void* pArg );
void                HashSaveStart    ( HWND hWndOwner, HSIMPLELIST hListInput );

UINT    CALLBACK HashPropCallback( HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
INT_PTR CALLBACK HashPropDlgProc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


// Activation context functions

ULONG_PTR      ActivateManifest   ( BOOL bActivate );
__inline void  DeactivateManifest ( ULONG_PTR uCookie )
{
   if (uCookie)
      DeactivateActCtx( 0, uCookie );
}
