#ifndef _SAVEICONVIEW_H_
#define _SAVEICONVIEW_H_

#include <boost/thread.hpp>
#include "win32/Window.h"
#include "../saves/Save.h"
#include "../MailBox.h"
#include "IconMesh.h"

class CSaveIconView : public Framework::Win32::CWindow
{
public:
										CSaveIconView(HWND, RECT*);
	virtual								~CSaveIconView();

	void								SetSave(const CSave*);
	void								SetIconType(CSave::ICONTYPE);

protected:
	long								OnLeftButtonDown(int, int);
	long								OnLeftButtonUp(int, int);
	long								OnMouseWheel(short);
	long								OnMouseMove(WPARAM, int, int);
	long								OnSetCursor(HWND, unsigned int, unsigned int);

private:
	void								ThreadProc();
	void								ThreadSetSave(const CSave*);
	void								ThreadSetIconType(CSave::ICONTYPE);
	void								LoadIcon();
	void								ChangeCursor();
	void								Render(HDC);
	void								DrawBackground();

	HGLRC								m_hRC;

	boost::thread*						m_thread;
	bool								m_threadOver;
	CMailBox							m_mailBox;

	static const PIXELFORMATDESCRIPTOR	m_PFD;
	const CSave*						m_save;
	CIconMesh*							m_iconMesh;
	CSave::ICONTYPE						m_iconType;
	
	float								m_nRotationX;
	float								m_nRotationY;

	bool								m_nGrabbing;
	int									m_nGrabPosX;
	int									m_nGrabPosY;
	int									m_nGrabDistX;
	int									m_nGrabDistY;
	float								m_nGrabRotX;
	float								m_nGrabRotY;
	float								m_nZoom;
};

#endif
