// fishtest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <atlstr.h>
#include <shlobj.h>
#include <stdarg.h>

#define MIN_DELAY_MSECS     50
#define DEF_DELAY_MSECS     750
#define MAX_DELAY_MSECS     5000

__inline void __cdecl MYTRACE( LPCTSTR pszFormat ... )
{
    CString   strTraceMsg;
    va_list   vargs;
    va_start( vargs, pszFormat );
    strTraceMsg.FormatV( pszFormat, vargs );
    _tprintf(_T("%s"), strTraceMsg );
    OutputDebugString( strTraceMsg );
    va_end( vargs );
}

int _tmain( int argc, _TCHAR* argv[] )
{
    CString strDllPath;
    int nDelayMSecs;
    int rc = 0;

    if (argc >= 3)
    {
        strDllPath = argv[1];
        if (!(nDelayMSecs = _ttoi( argv[2] )))
            nDelayMSecs = DEF_DELAY_MSECS;
    }
    else if (argc >= 2)
    {
        strDllPath = argv[1];
        nDelayMSecs = DEF_DELAY_MSECS;
    }
    else
    {
        MYTRACE(_T("*** fishtest: ERROR: not enough parameters passed!\n"));
        return -1;
    }

    if (nDelayMSecs < MIN_DELAY_MSECS ||
        nDelayMSecs > MAX_DELAY_MSECS)
        nDelayMSecs = DEF_DELAY_MSECS;

    nDelayMSecs += 250;

    MYTRACE(_T("*** fishtest: using nDelayMSecs = %d\n"), nDelayMSecs );

    Sleep( nDelayMSecs );
    SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL );
    Sleep( nDelayMSecs );
    SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL );
    Sleep( nDelayMSecs );

    if (!strDllPath.IsEmpty())
    {
        MYTRACE(_T("*** fishtest: DeleteFile( \"%s\" ) ...\n"), strDllPath );

        if (!DeleteFile( strDllPath ))
        {
            DWORD dwLastError = GetLastError();
            MYTRACE(_T("*** fishtest: ERROR: DeleteFile() FAILED: rc=%d (0x%08.8x)\n"),
                dwLastError, dwLastError );
            rc = (int) dwLastError;
        }
        else
            MYTRACE(_T("*** fishtest: DeleteFile() success!\n"));
    }
    else
    {
        MYTRACE(_T("*** fishtest: ERROR: 'strDllPath' is EMPTY!\n"));
        rc = -1;
    }

	return rc;
}
