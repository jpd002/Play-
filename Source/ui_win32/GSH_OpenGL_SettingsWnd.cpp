#include "resource.h"
#include "../gs/GSH_OpenGL/GSH_OpenGL.h"
#include "../AppConfig.h"
#include "GSH_OpenGL_SettingsWnd.h"

CGSH_OpenGL_SettingsWnd::CGSH_OpenGL_SettingsWnd(HWND parentWindow)
: CDialog(MAKEINTRESOURCE(IDD_GSHOPENGL_SETTINGS), parentWindow)
{
	SetClassPtr();

	m_linesAsQuadsCheck     = Framework::Win32::CButton(GetItem(IDC_GSHOPENGL_SETTINGS_LINESASQUAD));
	m_forceBilinearTextures = Framework::Win32::CButton(GetItem(IDC_GSHOPENGL_SETTINGS_FORCEBILINEAR));

	m_linesAsQuadsCheck.SetCheck(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS));
	m_forceBilinearTextures.SetCheck(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES));

	//Is this really needed?
	m_linesAsQuadsCheck.SetFocus();
}

CGSH_OpenGL_SettingsWnd::~CGSH_OpenGL_SettingsWnd()
{

}

long CGSH_OpenGL_SettingsWnd::OnCommand(unsigned short id, unsigned short cmd, HWND senderWnd)
{
	switch(id)
	{
	case IDOK:
		Save();
		Destroy();
		return TRUE;
		break;
	case IDCANCEL:
		Destroy();
		return TRUE;
		break;
	}
	return FALSE;
}

void CGSH_OpenGL_SettingsWnd::Save()
{
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS, m_linesAsQuadsCheck.GetCheck());
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, m_forceBilinearTextures.GetCheck());
}
