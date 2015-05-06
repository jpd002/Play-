#ifndef _ELFHEADERVIEW_H_
#define _ELFHEADERVIEW_H_

#include "win32/Dialog.h"
#include "win32/Edit.h"
#include "layout/GridLayout.h"
#include "../ELF.h"

class CELFHeaderView : public Framework::Win32::CDialog
{
public:
								CELFHeaderView(HWND, CELF*);
								~CELFHeaderView();

protected:
	long						OnSize(unsigned int, unsigned int, unsigned int);

private:
	void						FillInformation();
	void						RefreshLayout();
	CELF*						m_pELF;
	Framework::GridLayoutPtr    m_pLayout;
	Framework::Win32::CEdit*    m_pType;
	Framework::Win32::CEdit*    m_pMachine;
	Framework::Win32::CEdit*    m_pVersion;
	Framework::Win32::CEdit*    m_pEntry;
	Framework::Win32::CEdit*    m_pFlags;
	Framework::Win32::CEdit*    m_pSize;
	Framework::Win32::CEdit*    m_pPHOffset;
	Framework::Win32::CEdit*    m_pPHSize;
	Framework::Win32::CEdit*    m_pPHCount;
	Framework::Win32::CEdit*    m_pSHOffset;
	Framework::Win32::CEdit*    m_pSHSize;
	Framework::Win32::CEdit*    m_pSHCount;
	Framework::Win32::CEdit*    m_pSHStrTab;
};

#endif