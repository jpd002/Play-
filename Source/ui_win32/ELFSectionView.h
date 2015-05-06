#ifndef _ELFSECTIONVIEW_H_
#define _ELFSECTIONVIEW_H_

#include "win32/Dialog.h"
#include "win32/Edit.h"
#include "win32/Static.h"
#include "win32/ListView.h"
#include "layout/VerticalLayout.h"
#include "Types.h"
#include "MemoryViewPtr.h"
#include "../ELF.h"

class CELFSectionView : public Framework::Win32::CDialog
{
public:
									CELFSectionView(HWND, CELF*);
	virtual							~CELFSectionView();

	void							SetSectionIndex(uint16);

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnSetFocus();

private:
	void							RefreshLayout();
	void							FillInformation();

	void							CreateDynamicSectionListViewColumns();
	void							FillDynamicSectionListView();

	CELF*							m_pELF;
	uint16							m_nSection;

	Framework::FlatLayoutPtr		m_pLayout;
	Framework::Win32::CEdit*		m_pType;
	Framework::Win32::CEdit*		m_pFlags;
	Framework::Win32::CEdit*		m_pAddress;
	Framework::Win32::CEdit*		m_pOffset;
	Framework::Win32::CEdit*		m_pSize;
	Framework::Win32::CEdit*		m_pLink;
	Framework::Win32::CEdit*		m_pInfo;
	Framework::Win32::CEdit*		m_pAlignment;
	Framework::Win32::CEdit*		m_pEntrySize;
	Framework::Win32::CStatic*		m_contentsPlaceHolder;

	CMemoryViewPtr*					m_memoryView;
	Framework::Win32::CListView*	m_dynamicSectionListView;
};

#endif
