#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "../VideoDecoder.h"
#include "FrameBufferWindow.h"
#include "win32/Window.h"
#include <memory>

class CMainWindow : public Framework::Win32::CWindow
{
public:
	CMainWindow();
	virtual ~CMainWindow();

protected:
	long OnCommand(unsigned short, unsigned short, HWND);

private:
	void OnNewFrame(const Framework::CBitmap&);
	void OnFileOpen();

	CFrameBufferWindow*            m_frameBufferWindow;
	std::shared_ptr<CVideoDecoder> m_videoDecoder;
	boost::signals2::connection    m_onNewFrameConnection;
};

#endif
