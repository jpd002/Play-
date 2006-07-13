#ifndef _ELFSECTIONVIEW_H_
#define _ELFSECTIONVIEW_H_

#include "win32/Window.h"
#include "win32/Edit.h"
#include "VerticalLayout.h"
#include "Types.h"
#include "MemoryViewPtr.h"
#include "../ELF.h"

class CELFSectionView : public Framework::CWindow
{
public:
								CELFSectionView(HWND, CELF*, uint16);
								~CELFSectionView();
protected:
	long						OnSize(unsigned int, unsigned int, unsigned int);
	long						OnSetFocus();
private:
	void						RefreshLayout();
	void						FillInformation();
	CELF*						m_pELF;
	uint16						m_nSection;
	Framework::CVerticalLayout*	m_pLayout;
	Framework::Win32::CEdit*	m_pType;
	Framework::Win32::CEdit*	m_pFlags;
	Framework::Win32::CEdit*	m_pAddress;
	Framework::Win32::CEdit*	m_pOffset;
	Framework::Win32::CEdit*	m_pSize;
	Framework::Win32::CEdit*	m_pLink;
	Framework::Win32::CEdit*	m_pInfo;
	Framework::Win32::CEdit*	m_pAlignment;
	Framework::Win32::CEdit*	m_pEntrySize;
	CMemoryViewPtr*				m_pData;
};

#endif
