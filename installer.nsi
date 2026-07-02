!define PRODUCT_NAME "code-ZEN"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Sudip8900"
!define PRODUCT_WEB_SITE "https://github.com/Sudip8900/code-ZEN"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\code-ZEN.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

SetCompressor lzma

; MultiUser settings
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "InstallLocation"
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!include "MultiUser.nsh"
!include "MUI2.nsh"

; MUI Configuration
!define MUI_ABORTWARNING
!define MUI_ICON "resources\app_icon.ico"
!define MUI_UNICON "resources\app_icon.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "resources\installer_welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "resources\installer_welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "resources\installer_header.bmp"
!define MUI_UNHEADERIMAGE
!define MUI_UNHEADERIMAGE_BITMAP "resources\installer_header.bmp"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\code-ZEN.exe"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "code-ZEN-Installer.exe"
InstallDir "$PROGRAMFILES64\code-ZEN"

ShowInstDetails show
ShowUninstDetails show

Section "MainSection" SEC01
  ; Check if code-ZEN.exe exists in the target directory
  IfFileExists "$INSTDIR\code-ZEN.exe" check_running start_install

check_running:
  loop_check:
    ClearErrors
    FileOpen $0 "$INSTDIR\code-ZEN.exe" "a"
    IfErrors running not_running
  running:
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "code-ZEN is currently running in $INSTDIR. Please close the application and click Retry." IDRETRY loop_check
    Abort "Installation aborted by user because the application is running."
  not_running:
    FileClose $0

start_install:
  SetOutPath "$INSTDIR"
  SetOverwrite try
  File "build\Release\code-ZEN.exe"
  
  ; Copy resources
  SetOutPath "$INSTDIR\resources"
  File "resources\JetBrainsMono-Regular.ttf"
  File "resources\JetBrainsMono-Bold.ttf"
  File "resources\app_icon.ico"
  File "resources\app_icon.png"

  ; Shortcuts
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\code-ZEN.exe" "" "$INSTDIR\resources\app_icon.ico"
  CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\code-ZEN.exe" "" "$INSTDIR\resources\app_icon.ico"

  WriteUninstaller "$INSTDIR\uninstaller.exe"
  
  ; Write registry keys for uninstaller
  WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstaller.exe"'
  WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "DisplayIcon" '"$INSTDIR\resources\app_icon.ico"'
  WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr SHCTX "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
SectionEnd

Section -Post
  ; App Paths registry key
  WriteRegStr SHCTX "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\code-ZEN.exe"
  WriteRegStr SHCTX "${PRODUCT_DIR_REGKEY}" "Path" "$INSTDIR"
SectionEnd

Function .onInit
  !insertmacro MULTIUSER_INIT
  
  ; Check registry for existing installation
  ReadRegStr $0 SHCTX "${PRODUCT_UNINST_KEY}" "InstallLocation"
  StrCmp $0 "" no_install
    StrCpy $INSTDIR $0
  no_install:
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd

Section Uninstall
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  Delete "$INSTDIR\code-ZEN.exe"
  Delete "$INSTDIR\resources\JetBrainsMono-Regular.ttf"
  Delete "$INSTDIR\resources\JetBrainsMono-Bold.ttf"
  Delete "$INSTDIR\resources\app_icon.ico"
  Delete "$INSTDIR\resources\app_icon.png"
  RMDir "$INSTDIR\resources"
  
  Delete "$INSTDIR\uninstaller.exe"
  RMDir "$INSTDIR"

  DeleteRegKey SHCTX "${PRODUCT_UNINST_KEY}"
  DeleteRegKey SHCTX "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
