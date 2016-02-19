/**
 * Sets the Application User Model ID for windows or processes on NT 6.1+
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#pragma once

void SetAppIDForWindow( HWND hWnd, BOOL fEnable );
void SetAppIDForProcess();
