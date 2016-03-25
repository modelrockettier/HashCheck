@echo off
rem Clean builds both architectures (release mode)

pushd "%~dp0"
call "%VS120COMNTOOLS%\vsvars32"
echo Building 32-bit
echo.
msbuild /nologo HashCheck.vcxproj /p:Configuration=Release;Platform=Win32 /t:Clean;Build
if not errorlevel 0 exit /b 1
echo.

echo Building 64-bit
echo.
msbuild /nologo HashCheck.vcxproj /p:Configuration=Release;Platform=x64 /t:Clean;Build
if not errorlevel 0 exit /b 1
echo.

popd