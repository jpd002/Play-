#pragma once

#include "win32/Tab.h"
#include <vector>
#include <memory>

class CTabHost : public Framework::Win32::CWindow
{
public:
	enum
	{
		NOTIFICATION_SELCHANGED = 0xA000,
	};

	struct SELCHANGED_INFO : public NMHDR
	{
		uint32 selectedIndex;
		Framework::Win32::CWindow* selectedWindow;
	};

												CTabHost(HWND, const RECT&);
	virtual										~CTabHost();

	Framework::Win32::CWindow*					GetTab(unsigned int);
	void										InsertTab(const TCHAR*, Framework::Win32::CWindow*);

	int											GetSelection();
	void										SetSelection(unsigned int);

protected:
	long										OnSize(unsigned int, unsigned int, unsigned int) override;
	long										OnNotify(WPARAM, NMHDR*) override;

private:
	typedef std::vector<Framework::Win32::CWindow*> TabItemArray;

	void										OnTabSelChanged();

	std::unique_ptr<Framework::Win32::CTab>		m_tab;

	int											m_currentSelection;
	TabItemArray								m_tabItems;
};
