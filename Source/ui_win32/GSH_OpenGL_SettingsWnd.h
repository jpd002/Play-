#pragma once

#include "win32/Dialog.h"
#include "win32/Button.h"

class CGSH_OpenGL_SettingsWnd : public Framework::Win32::CDialog
{
public:
	           CGSH_OpenGL_SettingsWnd(HWND);
	virtual    ~CGSH_OpenGL_SettingsWnd();

protected:
	long    OnCommand(unsigned short, unsigned short, HWND);

private:
	void    Save();

	Framework::Win32::CButton    m_linesAsQuadsCheck;
	Framework::Win32::CButton    m_forceBilinearTextures;

	bool    m_nLinesAsQuads = false;
	bool    m_nForceBilinearTextures = false;
};
