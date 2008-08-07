#ifndef _PLAYERWND_H_
#define _PLAYERWND_H_

#include "PsxVm.h"
#include "win32/Window.h"
#include "AcceleratorTable.h"
#include "SpuRegView.h"
#include "PsfBase.h"

class CPlayerWnd : public Framework::Win32::CWindow
{
public:
											CPlayerWnd(CPsxVm&);
	virtual									~CPlayerWnd();

	void									Run();

protected:
	long									OnSize(unsigned int, unsigned int, unsigned int);
	long									OnCommand(unsigned short, unsigned short, HWND);
	long									OnTimer();

private:
	static HACCEL							CreateAccelerators();

	void									Load(const char*);
	void									PauseResume();
	void									ShowFileInformation();
	void									OnNewFrame();
	void									UpdateUi();


	CPsxVm&									m_virtualMachine;
	CSpuRegView*							m_regView;
	CPsfBase::TagMap						m_tags;
	unsigned int							m_frames;
	bool									m_ready;
	Framework::Win32::CAcceleratorTable		m_accel;
};

#endif
