#ifndef _ELFVIEWFRAME_H_
#define _ELFVIEWFRAME_H_

#include "win32/MDIFrame.h"
#include "ElfViewEx.h"

class CElfViewFrame : public Framework::Win32::CMDIFrame
{
public:
								CElfViewFrame(const char*);
    virtual						~CElfViewFrame();

protected:
	long						OnCommand(unsigned short, unsigned short, HWND);
	static HRESULT CALLBACK		TaskDialogCallback(HWND, UINT, WPARAM, LPARAM, LONG_PTR);

private:
	void						OpenElf();
    void						Load(const char*);
	void						CloseCurrentFile();
	void						ShowAboutBox();

	CElfViewEx*					m_elfView;
};

#endif
