@echo off
pushd "%~dp0"
call "%VS120COMNTOOLS%\vsvars32"
echo Building VersionTool
echo.
msbuild /nologo VersionTool\VersionTool.vcxproj /p:Configuration=Release /t:Clean;Build
echo.
echo Running VersionTool
echo.
VersionTool\Release\VersionTool "%~dp0"
popd
