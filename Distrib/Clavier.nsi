!include FileFunc.nsh
!include MUI2.nsh

!define LANG_NEUTRAL 0

!define APP_NAME "Clavier+"
!define APP_AUTHOR "Guillaume Ryder"
!define APP_URL "https://gryder.org/software/clavier-plus/"
!define APP_DOCUMENTATION_URL "https://gryder.org/software/clavier-plus/documentation"

!define APP_EXE "Clavier.exe"
!define APP_EXE_PATH "$InstDir\${APP_EXE}"

!define UNINSTALL_EXE "Uninstall.exe"
!define UNINSTALL_EXE_PATH "$InstDir\${UNINSTALL_EXE}"

!define UNINSTALL_REG_ROOT HKCU
!define UNINSTALL_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

!define AUTORUN_REG_ROOT HKCU
!define AUTORUN_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Run"

!define COMPIL_OUT_DIR "..\output\x64_Release"

!getdllversion "${COMPIL_OUT_DIR}\${APP_EXE}" APP_VERSION_
!define APP_VERSION_FULL "${APP_VERSION_1}.${APP_VERSION_2}.${APP_VERSION_3}.${APP_VERSION_4}"
!define APP_VERSION_DISPLAY "${APP_VERSION_1}.${APP_VERSION_2}.${APP_VERSION_3}"

# Dynamic name to omit the version number in some cases.
Var Name
Name $Name
!define APP_NAME_VERSIONED "${APP_NAME} ${APP_VERSION_DISPLAY}"

OutFile "ClavierSetup.exe"

InstallDir "$LOCALAPPDATA\${APP_NAME}"  ; default
InstallDirRegKey ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "InstallLocation"

RequestExecutionLevel user

ManifestDPIAware System
ManifestDPIAwareness "PerMonitorV2,System"

SetCompressor /FINAL /SOLID lzma

################################################################################
# Languages
################################################################################

# Calls a macro for each language with arguments: ISO code, LCID.
!macro ITER_LANGUAGES_FULL macro
  !insertmacro ${macro} "en" "English" # default
  !insertmacro ${macro} "de" "German"
  !insertmacro ${macro} "es" "Spanish"
  !insertmacro ${macro} "fr" "French"
  !insertmacro ${macro} "it" "Italian"
  !insertmacro ${macro} "hu" "Hungarian"
  !insertmacro ${macro} "nl" "Dutch"
  !insertmacro ${macro} "pl" "Polish"
  !insertmacro ${macro} "pt-BR" "PortugueseBR"
  !insertmacro ${macro} "sk" "Slovak"
  !insertmacro ${macro} "fi" "Finnish"
  !insertmacro ${macro} "el" "Greek"
  !insertmacro ${macro} "ru" "Russian"
  !insertmacro ${macro} "zh-TW" "TradChinese"
  !insertmacro ${macro} "zh-CN" "SimpChinese"
  !insertmacro ${macro} "ja" "Japanese"
!macroend
!macro DECLARE_LANGUAGE iso lcid
  !echo "Declaring language: ${iso} ${lcid}"
  !define LANG_${lcid}_ISO ${iso}
!macroend
!insertmacro ITER_LANGUAGES_FULL DECLARE_LANGUAGE

# Calls a macro for each language with the LCID as argument.
!macro ITER_LANGUAGES iter_languages_macro
  !insertmacro ITER_LANGUAGES_FULL ITER_LANGUAGES_CALL_MACRO_INTERNAL
!macroend
!macro ITER_LANGUAGES_CALL_MACRO_INTERNAL iso lcid
  !insertmacro ${iter_languages_macro} ${lcid}
!macroend

################################################################################
# Version information
################################################################################

VIProductVersion "${APP_VERSION_FULL}"
VIAddVersionKey /LANG=${LANG_NEUTRAL} "ProductName" "${APP_NAME}"
VIAddVersionKey /LANG=${LANG_NEUTRAL} "ProductVersion" "${APP_VERSION_FULL}"
VIAddVersionKey /LANG=${LANG_NEUTRAL} "FileDescription" "${APP_NAME}"
VIAddVersionKey /LANG=${LANG_NEUTRAL} "FileVersion" "${APP_VERSION_FULL}"
VIAddVersionKey /LANG=${LANG_NEUTRAL} "LegalCopyright" "${APP_AUTHOR}"

################################################################################
# MUI
################################################################################

# Remember the selected language
!define MUI_LANGDLL_REGISTRY_ROOT "${UNINSTALL_REG_ROOT}"
!define MUI_LANGDLL_REGISTRY_KEY "${UNINSTALL_REG_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Language"

# Install pages
!insertmacro MUI_PAGE_LICENSE "..\license.txt"

!insertmacro MUI_PAGE_DIRECTORY

Var StartMenuFolder
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${APP_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${UNINSTALL_REG_ROOT}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${UNINSTALL_REG_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"
!insertmacro MUI_PAGE_STARTMENU App $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "${APP_EXE_PATH}"
!define MUI_FINISHPAGE_RUN_PARAMETERS "/settings"
!insertmacro MUI_PAGE_FINISH

# Uninstall pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

# Settings
!define MUI_ABORTWARNING
!define MUI_LANGDLL_ALWAYSSHOW
!insertmacro ITER_LANGUAGES MUI_LANGUAGE

# Store required files first.
# Important with solid compression.
!insertmacro MUI_RESERVEFILE_LANGDLL

################################################################################
# Shared between the installer and the uninstaller.
#
# Macros needed to allow sharing.
# Functions overkill when called just once.
# https://nsis.sourceforge.io/Sharing_functions_between_Installer_and_Uninstaller
################################################################################

!macro SHARED_ONINIT
  StrCpy $Name "${APP_NAME_VERSIONED}"
!macroend

!define MSG_CLAVIER_NOTIFY_ICON "RyderClavierOptions"
!define IDCCMD_QUIT 1018
!define QUIT_POST_DELAY_MS 1000

!macro QUIT_APP
  System::Call "user32::RegisterWindowMessage(t'${MSG_CLAVIER_NOTIFY_ICON}') i.r0"
  SendMessage ${HWND_BROADCAST} $0 ${IDCCMD_QUIT} ${WM_COMMAND} /TIMEOUT=${QUIT_POST_DELAY_MS}
  Sleep ${QUIT_POST_DELAY_MS}
!macroend

################################################################################
# Installer
################################################################################

Function .onInit
  !insertmacro SHARED_ONINIT
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Section "Install"
  !insertmacro QUIT_APP

  SetOutPath "$InstDir"

  # Files
  File "${COMPIL_OUT_DIR}\${APP_EXE}"
  WriteUninstaller "${UNINSTALL_EXE_PATH}"

  # Start menu shortcuts
  StrCpy $Name "${APP_NAME}"
  !insertmacro MUI_STARTMENU_WRITE_BEGIN App
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk" "${APP_EXE_PATH}"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\$(MUI_UNTEXT_CONFIRM_TITLE).lnk" "${UNINSTALL_EXE_PATH}"
  !insertmacro MUI_STARTMENU_WRITE_END
  StrCpy $Name "${APP_NAME_VERSIONED}"

  # Default configuration file, language-dependent
  SetOverwrite off
  !macro LANG_SPECIFIC_CLAVIER_INI lcid
    StrCmp $LANGUAGE ${LANG_${lcid}} 0 +2
    File /oname=Clavier.ini "Clavier_${LANG_${lcid}_ISO}.ini"
  !macroend
  !insertmacro ITER_LANGUAGES LANG_SPECIFIC_CLAVIER_INI
  SetOverwrite on

  # Registry
  WriteRegStr ${AUTORUN_REG_ROOT} "${AUTORUN_REG_KEY}" "${APP_NAME}" '"${APP_EXE_PATH}" /launch'

  # Uninstall information
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "DisplayIcon" "${APP_EXE_PATH},0"
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "DisplayVersion" "${APP_VERSION_DISPLAY}"
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "HelpLink" "${APP_DOCUMENTATION_URL}"
  WriteRegDWORD ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "NoModify" 1
  WriteRegDWORD ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "NoRepair" 1
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "Publisher" "${APP_AUTHOR}"
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "QuietUninstallString" '"${UNINSTALL_EXE_PATH}" /S'
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "UninstallString" '"${UNINSTALL_EXE_PATH}"'
  WriteRegStr ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "URLInfoAbout" "${APP_URL}"
  WriteRegDWORD ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "VersionMajor" ${APP_VERSION_1}
  WriteRegDWORD ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "VersionMinor" ${APP_VERSION_2}

  # Estimated size
  ${GetSize} "$InstDir" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}" "EstimatedSize" "$0"
SectionEnd

################################################################################
# Uninstaller
################################################################################

Function un.onInit
  !insertmacro SHARED_ONINIT
  !insertmacro MUI_UNGETLANGUAGE
FunctionEnd

Section "Uninstall"
  !insertmacro QUIT_APP

  # Files - leave Clavier.ini
  Delete "${APP_EXE_PATH}"
  Delete "${UNINSTALL_EXE_PATH}"
  RMDir "$InstDir"

  # Start menu shortcuts
  StrCpy $Name "${APP_NAME}"
  !insertmacro MUI_STARTMENU_GETFOLDER App $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\$(MUI_UNTEXT_CONFIRM_TITLE).lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
  StrCpy $Name "${APP_NAME_VERSIONED}"

  # Registry
  DeleteRegValue ${AUTORUN_REG_ROOT} "${AUTORUN_REG_KEY}" "${APP_NAME}"
  DeleteRegKey ${UNINSTALL_REG_ROOT} "${UNINSTALL_REG_KEY}"
SectionEnd
