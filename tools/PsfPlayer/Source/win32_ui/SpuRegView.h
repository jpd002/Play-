#pragma once

#include "win32/Window.h"
#include "win32/ListView.h"
#include "iop/Iop_SpuBase.h"
#include "win32/ComPtr.h"
#include "win32/LayoutWindow.h"
#include "layout/VerticalLayout.h"
#include "win32/Static.h"
#include <thread>

class CSpuRegView : public Framework::Win32::CWindow
{
public:
							CSpuRegView(HWND, const RECT& rect, const TCHAR*);
	virtual					~CSpuRegView();

	void					SetSpu(Iop::CSpuBase*);

protected:
	virtual long			OnSize(unsigned int, unsigned int, unsigned int) override;

	void					Refresh();

private:
	void					CreateColumns();

	Iop::CSpuBase*			m_spu;

	std::tstring			m_title;

	Framework::Win32::CListView*	m_listView;
	Framework::Win32::CStatic*		m_extrainfo;
	Framework::FlatLayoutPtr		m_pLayout;

	std::thread*					m_thread;
	std::atomic<bool>				m_running;
};
