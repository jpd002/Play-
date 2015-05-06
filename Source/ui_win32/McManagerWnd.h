#pragma once

#include "win32/ModalWindow.h"
#include "win32/ComboBox.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include "MemoryCard.h"
#include "MemoryCardView.h"
#include "SaveView.h"
#include "../saves/SaveImporter.h"

class CMcManagerWnd : public Framework::Win32::CModalWindow, public boost::signals2::trackable
{
public:
												CMcManagerWnd(HWND);
	virtual										~CMcManagerWnd();

protected:
	long										OnCommand(unsigned short, unsigned short, HWND);

private:
	void										RefreshLayout();
	void										Import();
	void										Delete(const CSave*);
	void										UpdateMemoryCardSelection();
	CSaveImporterBase::OVERWRITE_PROMPT_RETURN	OnImportOverwrite(const boost::filesystem::path&);
	void										OnDeleteClicked(const CSave*);

	CMemoryCard									m_MemoryCard0;
	CMemoryCard									m_MemoryCard1;
	CMemoryCard*								m_pMemoryCard[2];
	CMemoryCard*								m_pCurrentMemoryCard;
	CMemoryCardView*							m_pMemoryCardView;
	CSaveView*									m_pSaveView;
	Framework::Win32::CComboBox*				m_pMemoryCardList;
	Framework::Win32::CButton*					m_pCloseButton;
	Framework::Win32::CButton*					m_pImportButton;
	Framework::FlatLayoutPtr					m_pLayout;
};
