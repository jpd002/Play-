#pragma once

#include "win32/Dialog.h"
#include "win32/Button.h"

class CGSH_OpenGL_SettingsWnd : public Framework::Win32::CDialog
{
public:
	CGSH_OpenGL_SettingsWnd(HWND);
	virtual ~CGSH_OpenGL_SettingsWnd();

protected:
	long OnCommand(unsigned short, unsigned short, HWND) override;

private:
	void Save();

	Framework::Win32::CButton m_enableHighResMode;
	Framework::Win32::CButton m_forceBilinearTextures;
};
