!include "MUI2.nsh"

!searchparse /file ../Source/AppDef.h '#define APP_VERSIONSTR _T("' APP_VERSION '")'

!define PROGRAM_NAME "ElfView"
!define DISPLAY_NAME "Elf Viewer"

; The name of the installer
Name "${DISPLAY_NAME} v${APP_VERSION}"

; The file to write
OutFile "${PROGRAM_NAME}-${APP_VERSION}-32.exe"

; The default installation directory
InstallDir $PROGRAMFILES\${PROGRAM_NAME}

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_${PROGRAM_NAME}" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

;!insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
!insertmacro MUI_LANGUAGE "English"
;--------------------------------

; Pages

;Page components
;Page directory
;Page instfiles

;UninstPage uninstConfirm
;UninstPage instfiles

;--------------------------------

!define REG_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}"

; The stuff to install
Section "${DISPLAY_NAME} (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\Win32\Release\ElfView.exe"
  File "..\changelog.html"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_${PROGRAM_NAME} "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayName" "${DISPLAY_NAME}"
  WriteRegStr HKLM "${REG_UNINSTALL}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoModify" 1
  WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoRepair" 1
  WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayIcon" '"$INSTDIR\ElfView.exe"'
  WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayVersion" "${APP_VERSION}"
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\${DISPLAY_NAME}"
  CreateShortCut "$SMPROGRAMS\${DISPLAY_NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${DISPLAY_NAME}\${DISPLAY_NAME}.lnk" "$INSTDIR\ElfView.exe" "" "$INSTDIR\ElfView.exe" 0
  CreateShortCut "$SMPROGRAMS\${DISPLAY_NAME}\changelog.html.lnk" "$INSTDIR\changelog.html" "" "$INSTDIR\changelog.html" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "${REG_UNINSTALL}"
  DeleteRegKey HKLM SOFTWARE\NSIS_${PROGRAM_NAME}

  ; Remove files and uninstaller
  Delete $INSTDIR\ElfView.exe
  Delete $INSTDIR\changelog.html
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\${DISPLAY_NAME}\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\${DISPLAY_NAME}"
  RMDir "$INSTDIR"

SectionEnd
