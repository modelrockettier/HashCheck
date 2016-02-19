@echo off

set "exe=%~f1"
set "dll=%~f2"
set "nnn=%~3"

echo Deleting DLL...
call :call_exe 11

echo Deleting EXE...
del "%exe%"

echo Deleting self and exiting...
del "%~f0" & exit /b 0

::--------------------------------------------------------

:call_exe

  set /a max=%1
  set /a try=1

:call_exe_retry

  echo "%exe%" "%dll%" %nnn%
       "%exe%" "%dll%" %nnn%
  set "rc=%errorlevel%"
  echo rc=%rc%

  if %rc%  EQU 0     goto :call_exe_success
  if %try% GEQ %max% goto :call_exe_failure

  echo Try #%try% failed. Waiting one minute before trying again...

  REM  192.0.2.1 guaranteed not to exist
  ping 192.0.2.1 -n 1 -w 60000

  set /a try+=1
  goto :call_exe_retry

:call_exe_success

  echo Try #%try% SUCCEEDED!
  goto :EOF

:call_exe_failure

  echo All %max% tries FAILED! Giving up!
  goto :EOF

::--------------------------------------------------------
