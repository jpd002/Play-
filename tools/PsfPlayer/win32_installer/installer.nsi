!include "MUI2.nsh"

!searchparse /file ../Source/AppDef.h '#define APP_VERSIONSTR _T("' APP_VERSION '")'

; The name of the installer
Name "PsfPlayer v${APP_VERSION}"

; The file to write
OutFile "PsfPlayer-${APP_VERSION}.exe"

; The default installation directory
InstallDir $PROGRAMFILES\PsfPlayer

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_PsfPlayer" "Install_Dir"

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
Section "PsfPlayer (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\Release\PsfPlayer.exe"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_PsfPlayer "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsfPlayer" "DisplayName" "PsfPlayer"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsfPlayer" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsfPlayer" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsfPlayer" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\PsfPlayer"
  CreateShortCut "$SMPROGRAMS\PsfPlayer\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\PsfPlayer\PsfPlayer.lnk" "$INSTDIR\PsfPlayer.exe" "" "$INSTDIR\PsfPlayer.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsfPlayer"
  DeleteRegKey HKLM SOFTWARE\NSIS_PsfPlayer

  ; Remove files and uninstaller
  Delete $INSTDIR\PsfPlayer.exe
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\PsfPlayer\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\PsfPlayer"
  RMDir "$INSTDIR"

SectionEnd
