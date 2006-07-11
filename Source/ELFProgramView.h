#ifndef _ELFPROGRAMVIEW_H_
#define _ELFPROGRAMVIEW_H_

#include "win32/Window.h"
#include "win32/Edit.h"
#include "GridLayout.h"
#include "Types.h"
#include "ELF.h"

class CELFProgramView : public Framework::CWindow
{
public:
								CELFProgramView(HWND, CELF*, uint16);
								~CELFProgramView();

protected:
	long						OnSize(unsigned int, unsigned int, unsigned int);

private:
	void						FillInformation();
	void						RefreshLayout();
	CELF*						m_pELF;
	uint16						m_nProgram;
	Framework::CGridLayout*		m_pLayout;
	Framework::Win32::CEdit*	m_pType;
	Framework::Win32::CEdit*	m_pOffset;
	Framework::Win32::CEdit*	m_pVAddr;
	Framework::Win32::CEdit*	m_pPAddr;
	Framework::Win32::CEdit*	m_pFileSize;
	Framework::Win32::CEdit*	m_pMemSize;
	Framework::Win32::CEdit*	m_pFlags;
	Framework::Win32::CEdit*	m_pAlign;
};

#endif