///////////////////////////////////////////////////////////////////////////////
//              Debugging macros (TRACE/ASSERT/VERIFY/etc)
///////////////////////////////////////////////////////////////////////////////

#pragma once

__inline  void  __cdecl             DebugTrace ( LPCTSTR pszFormat ... )
{
    CString   strTraceMsg;
    va_list   vargs;
    va_start( vargs, pszFormat );
    strTraceMsg.FormatV( pszFormat, vargs );
    OutputDebugString( strTraceMsg );
    va_end( vargs );
}

#define  TRACE                      DebugTrace
#define  TRACE_ALWAYS               DebugTrace

#define  BREAK_INTO_DEBUGGER        __debugbreak

#define  SRCLINE                    _T(__FILE__)           \
                                    _T("(")                \
                                    _T( QSTR( __LINE__ ))  \
                                    _T(")")

#define  ASSERT(a)                  \
  do                                \
  {                                 \
    if (!(a))                       \
    {                               \
      /* NOTE: message is formatted specifically for             \
         Visual Studio F4 next message compatibility */          \
      TRACE( SRCLINE                                             \
        _T(" : warning %s : %s : *** Assertion FAILED! ***\n"),  \
        _T(TARGETNAME),             \
        _T(__FUNCTION__) );         \
      if (IsDebuggerPresent())      \
        BREAK_INTO_DEBUGGER();      \
    }                               \
  }                                 \
  while(0)

#define  VERIFY                     ASSERT

///////////////////////////////////////////////////////////////////////////////

#define  HCTRACE(...)               \
                                    \
  TRACE( _T("*** ") _T(TARGETNAME)  \
    _T(" : ") SRCLINE               \
    _T(" : ") _T(__FUNCTION__)      \
    _T(" : ") __VA_ARGS__ )

///////////////////////////////////////////////////////////////////////////////

#define  APIFAILEDMSG( API )        \
                                    \
  do                                \
  {                                 \
    DWORD dwLastError;              \
    dwLastError = GetLastError();   \
    HCTRACE( _T("\"%s()\" FAILED : rc=%d (0x%08.8X) : %s\n"),  \
      _T(#API),                     \
      dwLastError,                  \
      dwLastError,                  \
      FormatLastError(dwLastError)  \
    );                              \
  }                                 \
  while(0)

///////////////////////////////////////////////////////////////////////////////

#define TRACEBOOLFUNC( FUNC, ... )  \
  do {                              \
    if (!FUNC( __VA_ARGS__ ))       \
      APIFAILEDMSG( FUNC );         \
  } while (0)

///////////////////////////////////////////////////////////////////////////////
