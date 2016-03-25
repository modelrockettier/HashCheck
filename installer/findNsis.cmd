@echo off
rem NSIS finding script by Ryan Pavlik
rem Somewhat inspired by the way vsvars32 works

set NSIS_ROOT=
call :getNSISHelper32 HKLM > nul 2>&1
if errorlevel 1 call :getNSISHelper32 HKCU > nul 2>&1
if errorlevel 1 call :getNSISHelper64 HKLM > nul 2>&1
if errorlevel 1 call :getNSISHelper64 HKCU > nul 2>&1

goto :eof
:getNSISHelper32
rem Retrieve the value from the registry
for /F "tokens=1,2*" %%i in ('reg query "%1\SOFTWARE\NSIS" /ve') DO (
	if "%%i"=="(Default)" (
		SET "NSIS_ROOT=%%k"
	)
)
rem Registry entry, but no file
if NOT EXIST "%NSIS_ROOT%\makensis.exe" (
	set NSIS_ROOT=
)
rem no luck
if "%NSIS_ROOT%"=="" exit /B 1
exit /B 0

:getNSISHelper64
rem Retrieve the value from the registry
for /F "tokens=1,2*" %%i in ('reg query "%1\SOFTWARE\Wow6432Node\NSIS" /ve') DO (
	if "%%i"=="(Default)" (
		SET "NSIS_ROOT=%%k"
	)
)
rem Registry entry, but no file
if NOT EXIST "%NSIS_ROOT%\makensis.exe" (
	set NSIS_ROOT=
)
rem no luck
if "%NSIS_ROOT%"=="" exit /B 1
exit /B 0