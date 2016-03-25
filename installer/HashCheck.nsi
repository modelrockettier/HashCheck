SetCompressor /FINAL /SOLID lzma

!include MUI2.nsh
!include x64.nsh

Unicode true

; Updated by the VersionTool app
!include "version_generated.nsh"

Name "HashCheck"
OutFile "HashCheckSetup-${APP_VER}.exe"

RequestExecutionLevel admin
ManifestSupportedOS all

!define MUI_ICON ..\HashCheck.ico
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_CUSTOMFUNCTION_SHOW change_license_font
!insertmacro MUI_PAGE_LICENSE "..\license.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Change to a smaller monospace font to more nicely display the license
;
Function change_license_font
    FindWindow $0 "#32770" "" $HWNDPARENT
    CreateFont $1 "Lucida Console" "7"
    GetDlgItem $0 $0 1000
    SendMessage $0 ${WM_SETFONT} $1 1
FunctionEnd

; These are the same languages supported by HashCheck
;
!insertmacro MUI_LANGUAGE "English" ; the first language is the default
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Ukrainian"

VIProductVersion "${APP_VER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "HashCheck Shell Extension"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${APP_VER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Installer distributed from ${APP_RELEASES_URL}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright � ${APP_AUTHORS}."
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "HashCheck installer (x86/x64), distributed from ${APP_RELEASES_URL}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${APP_VER}"

; With solid compression, files that are required before the
; actual installation should be stored first in the data block,
; because this will make the installer start faster.
;
!insertmacro MUI_RESERVEFILE_LANGDLL

; The install script
;
Section

    GetTempFileName $0
    File /oname=$0 ..\bin\Win32\Release\HashCheck.dll
    ExecWait 'regsvr32 /i /n /s "$0"'
    IfErrors abort_on_error
    Delete $0

    ; One of these exists and is undeletable if and only if
    ; it was in use and therefore a reboot is now required
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.0
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.1
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.2
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.3
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.4
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.5
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.6
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.7
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.8
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.9

    ${If} ${RunningX64}
        ${DisableX64FSRedirection}

        File /oname=$0 ..\bin\x64\Release\HashCheck.dll
        ExecWait 'regsvr32 /i /n /s "$0"'
        IfErrors abort_on_error
        Delete $0

        ; One of these exists and is undeletable if and only if
        ; it was in use and therefore a reboot is now required
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.0
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.1
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.2
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.3
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.4
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.5
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.6
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.7
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.8
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.9

        ${EnableX64FSRedirection}
    ${EndIf}

    Return

    abort_on_error:
        MessageBox MB_ICONSTOP|MB_OK "An unexpected error occurred during installation"

SectionEnd

Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd
