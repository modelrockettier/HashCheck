/**
 * Sets the Application User Model ID for windows or processes on NT 6.1+
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"

/*******************************************************************************
 *
 *
 * In order to build without requiring the installation of the NT 6.1 PSDK,
 * it is necessary to manually define a number of things...
 *
 *
 ******************************************************************************/

static const IID         IID_IPropertyStore               =   { 0x886d8eeb, 0x8cf2, 0x4446, { 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99 } };
static const PROPERTYKEY PKEY_AppUserModel_PreventPinning = { { 0x9f4c2855, 0x9f79, 0x4b39, { 0xa8, 0xd0, 0xe1, 0xd4, 0x2d, 0xe1, 0xd5, 0xf3 } }, 9 };
static const PROPERTYKEY PKEY_AppUserModel_ID             = { { 0x9f4c2855, 0x9f79, 0x4b39, { 0xa8, 0xd0, 0xe1, 0xd4, 0x2d, 0xe1, 0xd5, 0xf3 } }, 5 };

typedef HRESULT (WINAPI *PFNSCPEAUMID)( PCWSTR AppID );
typedef HRESULT (WINAPI *PFNSHGPSFW)( HWND hwnd, REFIID riid, void** ppv );


/*******************************************************************************
 *
 *
 * SetAppIDForWindow
 *
 *
 ******************************************************************************/

void SetAppIDForWindow( HWND hWnd, BOOL fEnable )
{
   if (g_wWinVer >= 0x601) // WIN7
   {
      IPropertyStore *pps;

      PFNSHGPSFW pfnSHGetPropertyStoreForWindow = (PFNSHGPSFW)
         GetProcAddress(GetModuleHandleA("SHELL32.dll"), "SHGetPropertyStoreForWindow");

      if ( pfnSHGetPropertyStoreForWindow &&
           SUCCEEDED(pfnSHGetPropertyStoreForWindow(hWnd, IID_IPropertyStore, (void**) &pps)) )
      {
         PROPVARIANT  propPreventPinning = {0};
         PROPVARIANT  propAppUserModelId = {0};

         if (fEnable)
         {
            // InitPropVariantFrom* functions were introduced in NT6, and
            // should be avoided because of the extra SDK dependency and
            // because of the extra COM memory allocation (and cleanup),
            // which is unnecessary since we are running in-process.

            propPreventPinning.vt      = VT_BOOL;
            propPreventPinning.boolVal = TRUE;

            propAppUserModelId.vt      = VT_LPWSTR;
            propAppUserModelId.pwszVal = APP_USER_MODEL_ID_W;
         }

         pps->SetValue(PKEY_AppUserModel_PreventPinning, propAppUserModelId);
         pps->SetValue(PKEY_AppUserModel_ID, propPreventPinning);

         pps->Release();
      }
   }
}


/*******************************************************************************
 *
 *
 * SetAppIDForProcess
 *
 *
 ******************************************************************************/

void SetAppIDForProcess()
{
   if (g_wWinVer >= 0x0601) // WIN7
   {
      PFNSCPEAUMID pfnSetCurrentProcessExplicitAppUserModelID = (PFNSCPEAUMID)
         GetProcAddress(GetModuleHandleA("SHELL32.dll"), "SetCurrentProcessExplicitAppUserModelID");

      if (pfnSetCurrentProcessExplicitAppUserModelID)
         VERIFY(SUCCEEDED(pfnSetCurrentProcessExplicitAppUserModelID(APP_USER_MODEL_ID_W)));
   }
}
