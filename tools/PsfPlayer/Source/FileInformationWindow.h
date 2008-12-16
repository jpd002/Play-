#ifndef _FILEINFORMATIONWINDOW_H_
#define _FILEINFORMATIONWINDOW_H_

#include "win32/ModalWindow.h"
#include "win32/Edit.h"
#include "win32/Layouts.h"
#include "PsfBase.h"

class CFileInformationWindow : public Framework::Win32::CModalWindow
{
public:
								CFileInformationWindow(HWND, const CPsfBase::TagMap&);
	virtual						~CFileInformationWindow();

protected:
	void						RefreshLayout();

private:
	std::string					GetTagValue(const char*);
	void						UpdateFields();

	CPsfBase::TagMap			m_tags;
	Framework::Win32::CEdit*	m_title;
	Framework::Win32::CEdit*	m_artist;
	Framework::Win32::CEdit*	m_game;
	Framework::Win32::CEdit*	m_year;
	Framework::Win32::CEdit*	m_genre;
	Framework::Win32::CEdit*	m_comment;
	Framework::Win32::CEdit*	m_copyright;
	Framework::Win32::CEdit*	m_psfBy;
	Framework::Win32::CEdit*	m_rawTags;
	Framework::LayoutObjectPtr	m_layout;
};

#endif
