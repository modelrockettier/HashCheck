@echo off
rem Builds and runs the version tool, clean builds both architectures, and builds the installer.

pushd "%~dp0"
call updateGeneratedVersionFiles
if not errorlevel 0 exit /b 1

call buildBoth
if not errorlevel 0 exit /b 1

pushd installer
call findNsis
if "%NSIS_ROOT%"=="" (
	echo Couldn't find an install of NSIS!
	pause
	exit /b 1
)
echo Building installer
echo.
"%NSIS_ROOT%\makensis" HashCheck.nsi
popd

popd
pause