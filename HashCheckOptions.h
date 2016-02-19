/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

// Options struct

struct HASHOPTIONS
{
    DWORD dwFlags;
    DWORD dwFilterIndex;
    DWORD dwMenuDisplay;
    DWORD dwSaveEncoding;
    LOGFONT lfFont;
};
typedef struct HASHOPTIONS HASHOPTIONS, *PHASHOPTIONS;

// Options flags

#define HCOF_FILTERINDEX  0x00000001  // The dwFilterIndex member is valid
#define HCOF_MENUDISPLAY  0x00000002  // The dwMenuDisplay member is valid
#define HCOF_SAVEENCODING 0x00000004  // The dwSaveEncoding member is valid
#define HCOF_FONT         0x00000008  // The lfFont member is valid
#define HCOF_ALL          0x0000000F

// Public functions

void __fastcall OptionsDialog( HWND hWndOwner, PHASHOPTIONS popt );
void __fastcall OptionsLoad( PHASHOPTIONS popt );
void __fastcall OptionsSave( PHASHOPTIONS popt );
bool  OptionsDelete ();
