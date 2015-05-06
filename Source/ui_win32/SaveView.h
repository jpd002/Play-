#pragma once

#include "SaveIconView.h"
#include <boost/signals2.hpp>
#include "win32/Window.h"
#include "win32/Edit.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include "CommandSink.h"
#include "../saves/Save.h"

class CSaveView : public Framework::Win32::CWindow, public boost::signals2::trackable
{
public:
	typedef boost::signals2::signal<void (const CSave*)> DeleteClickSignal;

										CSaveView(HWND);
	virtual								~CSaveView();

	void								SetSave(const CSave*);

	DeleteClickSignal					OnDeleteClick;

protected:
	long								OnSize(unsigned int, unsigned int, unsigned int);
	long								OnCommand(unsigned short, unsigned short, HWND);

private:
	void								RefreshLayout();

	long								SetIconType(CSave::ICONTYPE);
	long								OpenSaveFolder();
	long								Export();
	long								Delete();

	CCommandSink						m_CommandSink;

	const CSave*						m_pSave;
	Framework::FlatLayoutPtr			m_pLayout;
	Framework::Win32::CEdit*			m_pNameLine1;
	Framework::Win32::CEdit*			m_pNameLine2;
	Framework::Win32::CEdit*			m_pSize;
	Framework::Win32::CEdit*			m_pId;
	Framework::Win32::CEdit*			m_pLastModified;
	Framework::Win32::CButton*			m_pOpenFolder;
	Framework::Win32::CButton*			m_pExport;
	Framework::Win32::CButton*			m_pDelete;
	Framework::Win32::CButton*			m_pNormalIcon;
	Framework::Win32::CButton*			m_pCopyingIcon;
	Framework::Win32::CButton*			m_pDeletingIcon;
	CSaveIconView*						m_pIconViewWnd;
	CSave::ICONTYPE						m_nIconType;
};
