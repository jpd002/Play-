!include "MUI2.nsh"

!searchparse /file ../Source/AppDef.h '#define APP_VERSIONSTR _T("' APP_VERSION '")'

; The name of the installer
Name "Purei! v${APP_VERSION}"

; The file to write
OutFile "Purei-${APP_VERSION}.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Purei

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_Purei" "Install_Dir"

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

; The stuff to install
Section "Purei! (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\Release\Purei.exe"
  File "..\Readme.html"
  File "..\Patches.xml"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_Purei "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Purei" "DisplayName" "Purei"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Purei" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Purei" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Purei" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Purei!"
  CreateShortCut "$SMPROGRAMS\Purei!\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Purei!\Purei!.lnk" "$INSTDIR\Purei.exe" "" "$INSTDIR\Purei.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Purei"
  DeleteRegKey HKLM SOFTWARE\NSIS_Purei

  ; Remove files and uninstaller
  Delete $INSTDIR\Purei.exe
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Purei!\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Purei!"
  RMDir "$INSTDIR"

SectionEnd
