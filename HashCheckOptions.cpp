/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "stdafx.h"
#include "HashCheckOptions.h"
#include "RegHelpers.h"
#include "libs\IsFontAvailable.h"

#define OPTIONS_KEYNAME _T("Software\\HashCheck")

struct OPTIONSCONTEXT {
   PHASHOPTIONS popt;
   HFONT hFont;
   BOOL bFontChanged;
};
typedef struct OPTIONSCONTEXT OPTIONSCONTEXT, *POPTIONSCONTEXT;

static const LOGFONT lfHCLogFont =
{
   0,                          // lfHeight (calculated from HC_FONT_POINT_SIZE)
   0,                          // lfWidth
   0,                          // lfEscapement
   0,                          // lfOrientation
   FW_REGULAR,                 // lfWeight
   FALSE,                      // lfItalic
   FALSE,                      // lfUnderline
   FALSE,                      // lfStrikeOut
   DEFAULT_CHARSET,            // lfCharSet
   OUT_DEFAULT_PRECIS,         // lfOutPrecision
   CLIP_DEFAULT_PRECIS,        // lfClipPrecision
   DEFAULT_QUALITY,            // lfQuality
   FIXED_PITCH | FF_DONTCARE,  // lfPitchAndFamily
   HC_FONT_DEF_NAME            // lfFaceName[LF_FACESIZE]
};



/*============================================================================*\
   Function declarations
\*============================================================================*/

// Dialog general
INT_PTR CALLBACK OptionsDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void OptionsDlgInit( HWND hWnd, POPTIONSCONTEXT poptctx );

// Dialog commands
void ChangeFont( HWND hWnd, POPTIONSCONTEXT poptctx );
void SaveSettings( HWND hWnd, POPTIONSCONTEXT poptctx );



/*============================================================================*\
   Public functions
\*============================================================================*/

void __fastcall OptionsDialog( HWND hWndOwner, PHASHOPTIONS popt )
{
   // First, activate our manifest
   ULONG_PTR uActCtxCookie = ActivateManifest(TRUE);

   // Initialize the dialog's context
   OPTIONSCONTEXT optctx = { popt, NULL, FALSE };

   // Show the options dialog; returning the results to our caller is handled
   // by the flags set in the caller's popt
   DialogBoxParam(
      g_hModThisDll,
      MAKEINTRESOURCE(IDD_OPTIONS),
      hWndOwner,
      OptionsDlgProc,
      (LPARAM)&optctx
   );

   // Release the font handle, if the dialog created one
   if (optctx.hFont) DeleteObject(optctx.hFont);

   // Clean up the manifest activation
   DeactivateManifest(uActCtxCookie);
}

void __fastcall OptionsLoad( PHASHOPTIONS popt )
{
   HKEY hKey;

   if (RegOpenKeyEx(HKEY_CURRENT_USER, OPTIONS_KEYNAME, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
      hKey = NULL;

   if (popt->dwFlags & HCOF_FILTERINDEX)
   {
      if (!( hKey &&
             HCRegGetDWORD(hKey, _T("FilterIndex"), &popt->dwFilterIndex) &&
             popt->dwFilterIndex && popt->dwFilterIndex <= NUM_HASH_TYPES ))
      {
         // Fall back to default (MD5)
         popt->dwFilterIndex = 3;
      }
   }

   if (popt->dwFlags & HCOF_MENUDISPLAY)
   {
      if (!( hKey &&
             HCRegGetDWORD(hKey, _T("MenuDisplay"), &popt->dwMenuDisplay) &&
             popt->dwMenuDisplay < 3 ))
      {
         // Fall back to default (always show)
         popt->dwMenuDisplay = 0;
      }
   }

   if (popt->dwFlags & HCOF_SAVEENCODING)
   {
      if (!( hKey &&
             HCRegGetDWORD(hKey, _T("SaveEncoding"), &popt->dwSaveEncoding) &&
             popt->dwSaveEncoding < 3 ))
      {
         // Fall back to default (UTF-8)
         popt->dwSaveEncoding = 0;
      }
   }

   if (popt->dwFlags & HCOF_FONT)
   {
      DWORD dwType;
      DWORD cbData = sizeof(LOGFONT);

      if (!( hKey && ERROR_SUCCESS ==
             RegQueryValueEx(hKey, _T("Font"), NULL, &dwType, (PBYTE)&popt->lfFont, &cbData) &&
             dwType == REG_BINARY && cbData == sizeof(LOGFONT)) )
      {
         HDC hDC;

         // Copy the default font (Lucida Console)
         memcpy(&popt->lfFont, &lfHCLogFont, sizeof(LOGFONT));

         // Use Consolas if it is available
         if (IsFontAvailable(HC_FONT_PREF_NAME))
         {
            popt->lfFont.lfQuality = CLEARTYPE_QUALITY;
            SSChainNCpyW(popt->lfFont.lfFaceName, HC_FONT_PREF_NAME, _countof(HC_FONT_PREF_NAME));
         }

         // Convert PointSize to device units (i.e. pixels, usually)
         hDC = GetDC(NULL);
         popt->lfFont.lfHeight = -MulDiv(HC_FONT_POINT_SIZE, GetDeviceCaps(hDC, LOGPIXELSY), 72);
         ReleaseDC(NULL, hDC);
      }
   }

   if (hKey)
      RegCloseKey(hKey);
}

void __fastcall OptionsSave( PHASHOPTIONS popt )
{
   HKEY hKey;

   // Avoid creating the key if nothing is being set
   if (!popt->dwFlags)
      return;

   if (hKey = HCRegCreateOpen(HKEY_CURRENT_USER, OPTIONS_KEYNAME, NULL))
   {
      if (popt->dwFlags & HCOF_FILTERINDEX)
         HCRegPutDWORD(hKey, _T("FilterIndex"), popt->dwFilterIndex);

      if (popt->dwFlags & HCOF_MENUDISPLAY)
         HCRegPutDWORD(hKey, _T("MenuDisplay"), popt->dwMenuDisplay);

      if (popt->dwFlags & HCOF_SAVEENCODING)
         HCRegPutDWORD(hKey, _T("SaveEncoding"), popt->dwSaveEncoding);

      if (popt->dwFlags & HCOF_FONT)
         RegSetValueEx(hKey, _T("Font"), 0, REG_BINARY, (PBYTE)&popt->lfFont, sizeof(LOGFONT));

      RegCloseKey(hKey);
   }
}



/*============================================================================*\
   Dialog general
\*============================================================================*/

INT_PTR CALLBACK OptionsDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         SetWindowLongPtr(hWnd, DWLP_USER, lParam);
         OptionsDlgInit(hWnd, (POPTIONSCONTEXT)lParam);
         return(TRUE);
      }

      case WM_ENDSESSION:
      case WM_CLOSE:
      {
         goto end_dialog;
      }

      case WM_COMMAND:
      {
         POPTIONSCONTEXT poptctx = (POPTIONSCONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);

         switch (LOWORD(wParam))
         {
            case IDC_OPT_FONT_CHANGE:
               ChangeFont(hWnd, poptctx);
               return(TRUE);

            case IDC_OK:
               SaveSettings(hWnd, poptctx);
            case IDC_CANCEL:
               end_dialog: EndDialog(hWnd, 0);
               return(TRUE);
         }

         break;
      }
   }

   return(FALSE);
}

void OptionsDlgInit( HWND hWnd, POPTIONSCONTEXT poptctx )
{
   // Load strings
   {
      static const UINT16 arStrMap[][2] =
      {
         { IDC_OPT_CM,             IDS_OPT_CM             },
         { IDC_OPT_CM_ALWAYS,      IDS_OPT_CM_ALWAYS      },
         { IDC_OPT_CM_EXTENDED,    IDS_OPT_CM_EXTENDED    },
         { IDC_OPT_CM_NEVER,       IDS_OPT_CM_NEVER       },
         { IDC_OPT_ENCODING,       IDS_OPT_ENCODING       },
         { IDC_OPT_ENCODING_UTF8,  IDS_OPT_ENCODING_UTF8  },
         { IDC_OPT_ENCODING_UTF16, IDS_OPT_ENCODING_UTF16 },
         { IDC_OPT_ENCODING_ANSI,  IDS_OPT_ENCODING_ANSI  },
         { IDC_OPT_FONT,           IDS_OPT_FONT           },
         { IDC_OPT_FONT_CHANGE,    IDS_OPT_FONT_CHANGE    },
         { IDC_OK,                 IDS_OPT_OK             },
         { IDC_CANCEL,             IDS_OPT_CANCEL         }
      };

      static const TCHAR szFormat[] = _T("%s (v") VERSION_STR _T("/") ARCH_NAME _T(")");

      TCHAR szBuffer[MAX_STRINGMSG], szTitle[MAX_STRINGMSG];
      UINT i;

      for (i = 0; i < _countof(arStrMap); ++i)
         SetControlText(hWnd, arStrMap[i][0], arStrMap[i][1]);

      LoadString(g_hModThisDll, IDS_OPT_TITLE, szBuffer, _countof(szBuffer));
      wnsprintf(szTitle, _countof(szTitle), szFormat, szBuffer);
      SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szTitle);
   }

   // Load options
   {
      poptctx->popt->dwFlags = HCOF_ALL;
      OptionsLoad(poptctx->popt);
      poptctx->popt->dwFlags = 0;

      SendDlgItemMessage(hWnd, IDC_OPT_CM_FIRSTID + poptctx->popt->dwMenuDisplay,
                         BM_SETCHECK, BST_CHECKED, 0);

      SendDlgItemMessage(hWnd, IDC_OPT_ENCODING_FIRSTID + poptctx->popt->dwSaveEncoding,
                         BM_SETCHECK, BST_CHECKED, 0);

      if (poptctx->hFont = CreateFontIndirect(&poptctx->popt->lfFont))
         SendDlgItemMessage(hWnd, IDC_OPT_FONT_PREVIEW, WM_SETFONT, (WPARAM)poptctx->hFont, FALSE);

      SetDlgItemText(hWnd, IDC_OPT_FONT_PREVIEW, poptctx->popt->lfFont.lfFaceName);
   }
}



/*============================================================================*\
   Dialog commands
\*============================================================================*/

void ChangeFont( HWND hWnd, POPTIONSCONTEXT poptctx )
{
   HFONT hFont;

   CHOOSEFONT cf;
   cf.lStructSize = sizeof(cf);
   cf.hwndOwner = hWnd;
   cf.lpLogFont = &poptctx->popt->lfFont;
   cf.Flags = CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

   if (ChooseFont(&cf) && (hFont = CreateFontIndirect(&poptctx->popt->lfFont)))
   {
      // Signal that a new font should be saved if "OK" is pressed
      poptctx->bFontChanged = TRUE;

      // Update the preview
      SendDlgItemMessage(hWnd, IDC_OPT_FONT_PREVIEW, WM_SETFONT, (WPARAM)hFont, FALSE);
      SetDlgItemText(hWnd, IDC_OPT_FONT_PREVIEW, poptctx->popt->lfFont.lfFaceName);

      // Clean up
      if (poptctx->hFont) DeleteObject(poptctx->hFont);
      poptctx->hFont = hFont;
   }
}

void SaveSettings( HWND hWnd, POPTIONSCONTEXT poptctx )
{
   DWORD i;

   // Get the user-selected value for dwMenuDisplay
   for (i = 0; i < 3; ++i)
   {
      if (SendDlgItemMessage(hWnd, IDC_OPT_CM_FIRSTID + i, BM_GETCHECK, 0, 0) == BST_CHECKED)
         break;
   }

   // If the value has changed, flag it for saving
   if (i < 3 && poptctx->popt->dwMenuDisplay != i)
   {
      poptctx->popt->dwFlags |= HCOF_MENUDISPLAY;
      poptctx->popt->dwMenuDisplay = i;
   }

   // Get the user-selected value for dwSaveEncoding
   for (i = 0; i < 3; ++i)
   {
      if (SendDlgItemMessage(hWnd, IDC_OPT_ENCODING_FIRSTID + i, BM_GETCHECK, 0, 0) == BST_CHECKED)
         break;
   }

   // If the value has changed, flag it for saving
   if (i < 3 && poptctx->popt->dwSaveEncoding != i)
   {
      poptctx->popt->dwFlags |= HCOF_SAVEENCODING;
      poptctx->popt->dwSaveEncoding = i;
   }

   // Flag the font for saving if necessary
   if (poptctx->bFontChanged)
      poptctx->popt->dwFlags |= HCOF_FONT;

   OptionsSave(poptctx->popt);
}

///////////////////////////////////////////////////////////////////////////////

bool OptionsDelete()
{
    return HCRegDeleteTree( HKEY_CURRENT_USER, USER_OPTIONS_KEY );
}

///////////////////////////////////////////////////////////////////////////////
