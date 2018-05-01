#ifndef _FILEINFORMATIONPANEL_H_
#define _FILEINFORMATIONPANEL_H_

#include "win32/Dialog.h"
#include "win32/Edit.h"
#include "win32/Layouts.h"
#include "PsfTags.h"

class CFileInformationPanel : public Framework::Win32::CDialog
{
public:
	CFileInformationPanel(HWND);
	virtual ~CFileInformationPanel();

	void SetTags(const CPsfTags&);
	void RefreshLayout();

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;

private:
	void UpdateFields();

	Framework::Win32::CEdit* m_title;
	Framework::Win32::CEdit* m_artist;
	Framework::Win32::CEdit* m_game;
	Framework::Win32::CEdit* m_year;
	Framework::Win32::CEdit* m_genre;
	Framework::Win32::CEdit* m_comment;
	Framework::Win32::CEdit* m_copyright;
	Framework::Win32::CEdit* m_psfBy;
	Framework::Win32::CEdit* m_rawTags;

	CPsfTags m_tags;

	Framework::LayoutObjectPtr m_layout;
};

#endif
