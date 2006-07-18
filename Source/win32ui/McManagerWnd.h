#ifndef _MEMORYCARDMANAGER_H_
#define _MEMORYCARDMANAGER_H_

#include "ModalWindow.h"
#include "win32/ComboBox.h"
#include "win32/Button.h"
#include "MemoryCard.h"
#include "MemoryCardView.h"
#include "VerticalLayout.h"
#include "SaveView.h"

class CMcManagerWnd : public CModalWindow
{
public:
									CMcManagerWnd(HWND);
	virtual							~CMcManagerWnd();

protected:
	long							OnDrawItem(unsigned int, LPDRAWITEMSTRUCT);
	long							OnCommand(unsigned short, unsigned short, HWND);

private:
	void							RefreshLayout();
	void							Import();

	CMemoryCard						m_MemoryCard0;
	CMemoryCardView*				m_pMemoryCardView;
	CSaveView*						m_pSaveView;
	Framework::Win32::CComboBox*	m_pMemoryCardList;
	Framework::Win32::CButton*		m_pImportButton;
	Framework::CVerticalLayout*		m_pLayout;
};

#endif
