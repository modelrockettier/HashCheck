///////////////////////////////////////////////////////////////////////////////////////
// product.h  --  #defines constants identifying the product itself
///////////////////////////////////////////////////////////////////////////////////////

#pragma once

//-------------------------------------------------------------------------------------
//  Please refer to README.txt for information about this source code.
//  Please refer to LICENSE.txt for details about distribution and modification.
//-------------------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////////////////////////
// The following are constant values that rarely change over time...

static  const         GUID
        PRODUCT_CLSID_GUID  = { 0x705977c7, 0x86cb, 0x4743, { 0xbf, 0xaf, 0x69, 0x08, 0xbd, 0x19, 0xb7, 0xb0 } };
#define PRODUCT_CLSID_A         "{705977C7-86CB-4743-BFAF-6908BD19B7B0}"
#define COPYRIGHT_YEAR_A        "Copyright (C) 2014, "
#define COMPANY_A               "Software Development Laboratories"
#define PRODUCT_NAME_A          "HashCheck"
#define PRODUCT_DESC_A          "HashCheck Shell Extension"
#define PRODUCT_COMMENT_A       "Based on Kai Liu's original code at http://code.kliu.org"

// (same as above, but as _T() strings)

#define PRODUCT_CLSID           _T(PRODUCT_CLSID_A)
#define COPYRIGHT_YEAR          _T(COPYRIGHT_YEAR_A)
#define COMPANY                 _T(COMPANY_A)
#define PRODUCT_NAME            _T(PRODUCT_NAME_A)
#define PRODUCT_DESC            _T(PRODUCT_DESC_A)
#define PRODUCT_COMMENT         _T(PRODUCT_COMMENT_A)

///////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous product strings

#define APP_USER_MODEL_ID_W       L"SDL."      QWSTR(PRODUCT_NAME_A)
#define APP_USER_MODEL_ID       _T("SDL.")        _T(PRODUCT_NAME_A)

#define USER_OPTIONS_KEY_A         "Software\\"      PRODUCT_NAME_A
#define USER_OPTIONS_KEY        _T("Software\\")  _T(PRODUCT_NAME_A)

///////////////////////////////////////////////////////////////////////////////////////
