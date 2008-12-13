#ifndef _PLAYERWND_H_
#define _PLAYERWND_H_

#include "PsfVm.h"
#include "win32/Window.h"
#include "AcceleratorTable.h"
#include "SpuRegView.h"
#include "PsfBase.h"

class CPlayerWnd : public Framework::Win32::CWindow
{
public:
											CPlayerWnd(CPsfVm&);
	virtual									~CPlayerWnd();

	void									Run();

protected:
	long									OnWndProc(unsigned int, WPARAM, LPARAM);
	long									OnSize(unsigned int, unsigned int, unsigned int);
	long									OnCommand(unsigned short, unsigned short, HWND);
	long									OnTimer();

private:
    enum
    {
        MAX_CORE = 2
    };

	static HACCEL							CreateAccelerators();

	void									Load(const char*);
	void									PauseResume();
	void									ShowFileInformation();
	void									EnableReverb();
	void									ShowAbout();
	void									OnNewFrame();
	void									UpdateFromConfig();
	void									UpdateReverbStatus();
	void									UpdateTitle();
	void									UpdateMenu();

	CPsfVm&									m_virtualMachine;
	CSpuRegView*							m_regView[MAX_CORE];
	CPsfBase::TagMap						m_tags;
	unsigned int							m_frames;
	bool									m_ready;
	Framework::Win32::CAcceleratorTable		m_accel;
};

#endif
