;Inspired by PCSX2's Web Installer Script

!define REDIST_NAME "Visual C++ 2013 Redistributable"
!define REDIST_SETUP_FILENAME "vcredist_x86.exe"

Section "${REDIST_NAME}" SEC_CRT2013

  SectionIn RO

  ClearErrors
  ReadRegDword $R0 HKLM "SOFTWARE\Microsoft\VisualStudio\12.0\VC\Runtimes\x86" "Installed"
  IfErrors 0 +2
  DetailPrint "Installing ${REDIST_NAME}..."
  StrCmp $R0 "1" 0 +3
    DetailPrint "${REDIST_NAME} is already installed. Skipping."
    Goto done

  SetOutPath "$TEMP"

  DetailPrint "Downloading ${REDIST_NAME} Setup..."
  NSISdl::download /TIMEOUT=15000 "http://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x86.exe" "${REDIST_SETUP_FILENAME}"

  Pop $R0 ;Get the return value
  StrCmp $R0 "success" OnSuccess

  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +2
    MessageBox MB_OK "Could not download ${REDIST_NAME} Setup."
    Goto done

OnSuccess:
  DetailPrint "Running ${REDIST_NAME} Setup..."
  ExecWait '"$TEMP\${REDIST_SETUP_FILENAME}" /q /norestart'
  DetailPrint "Finished ${REDIST_NAME} Setup"
  
  Delete "$TEMP\${REDIST_SETUP_FILENAME}"

done:
SectionEnd
