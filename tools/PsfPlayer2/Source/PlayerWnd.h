#ifndef _PLAYERWND_H_
#define _PLAYERWND_H_

#include "PsxVm.h"
#include "win32/Window.h"
#include "SpuRegView.h"

class CPlayerWnd : public Framework::Win32::CWindow
{
public:
					CPlayerWnd(CPsxVm&);
	virtual			~CPlayerWnd();

protected:
	long			OnSize(unsigned int, unsigned int, unsigned int);
	long			OnCommand(unsigned short, unsigned short, HWND);
	long			OnTimer();

private:
	void			Load(const char*);
	void			OnNewFrame();

	CPsxVm&			m_virtualMachine;
	CSpuRegView*	m_regView;
	unsigned int	m_frames;
};

#endif
