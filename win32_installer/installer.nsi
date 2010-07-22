!include "MUI2.nsh"

!searchparse /file ../Source/AppDef.h '#define APP_VERSIONSTR _T("' APP_VERSION '")'

; The name of the installer
Name "Play! v${APP_VERSION}"

; The file to write
OutFile "Play-${APP_VERSION}.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Play

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_Play" "Install_Dir"

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
Section "Play! (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\win32\Release\Play.exe"
  File "..\Readme.html"
  File "..\Patches.xml"
  File "..\icudt34.dll"
  File "..\icuuc34.dll"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_Play "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Play" "DisplayName" "Play"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Play" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Play" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Play" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Play!"
  CreateShortCut "$SMPROGRAMS\Play!\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Play!\Play!.lnk" "$INSTDIR\Play.exe" "" "$INSTDIR\Play.exe" 0
  CreateShortCut "$SMPROGRAMS\Play!\Read Me.lnk" "$INSTDIR\Readme.html" "" "$INSTDIR\Readme.html" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Play"
  DeleteRegKey HKLM SOFTWARE\NSIS_Play

  ; Remove files and uninstaller
  Delete $INSTDIR\Play.exe
  Delete $INSTDIR\Readme.html
  Delete $INSTDIR\Patches.xml
  Delete $INSTDIR\icudt34.dll
  Delete $INSTDIR\icuuc34.dll
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Play!\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Play!"
  RMDir "$INSTDIR"

SectionEnd
