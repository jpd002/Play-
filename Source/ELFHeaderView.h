#ifndef _ELFHEADERVIEW_H_
#define _ELFHEADERVIEW_H_

#include "win32/Window.h"
#include "win32/Edit.h"
#include "GridLayout.h"
#include "ELF.h"

class CELFHeaderView : public Framework::CWindow
{
public:
							CELFHeaderView(HWND, CELF*);
							~CELFHeaderView();

protected:
	long					OnSize(unsigned int, unsigned int, unsigned int);

private:
	void					FillInformation();
	void					RefreshLayout();
	CELF*					m_pELF;
	Framework::CGridLayout*	m_pLayout;
	Framework::CEdit*		m_pType;
	Framework::CEdit*		m_pMachine;
	Framework::CEdit*		m_pVersion;
	Framework::CEdit*		m_pEntry;
	Framework::CEdit*		m_pFlags;
	Framework::CEdit*		m_pSize;
	Framework::CEdit*		m_pPHOffset;
	Framework::CEdit*		m_pPHSize;
	Framework::CEdit*		m_pPHCount;
	Framework::CEdit*		m_pSHOffset;
	Framework::CEdit*		m_pSHSize;
	Framework::CEdit*		m_pSHCount;
	Framework::CEdit*		m_pSHStrTab;
};

#endif