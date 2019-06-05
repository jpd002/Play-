#pragma once

#include <boost/signals2.hpp>
#include <functional>
#include "win32/MDIChild.h"
#include "win32/ListView.h"
#include "win32/Button.h"
#include "layout/VerticalLayout.h"
#include "../MIPS.h"
#include "../BiosDebugInfoProvider.h"
#include "../ELF.h"

class CFunctionsView :
#ifdef FUNCTIONSVIEW_STANDALONE
    public Framework::Win32::CWindow,
#else
    public Framework::Win32::CMDIChild,
#endif
    public boost::signals2::trackable
{
public:
	CFunctionsView(HWND);
	virtual ~CFunctionsView() = default;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);
	void Refresh();

	boost::signals2::signal<void(uint32)> OnFunctionDblClick;
	boost::signals2::signal<void(void)> OnFunctionsStateChange;

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;
	long OnCommand(unsigned short, unsigned short, HWND) override;
	LRESULT OnNotify(WPARAM, NMHDR*) override;
	long OnSysCommand(unsigned int, LPARAM) override;

private:
	void CreateListColumns();
	void ResizeListColumns();
	void RefreshLayout();
	void RefreshList();
	void InitializeModuleGrouper();
	uint32 GetFunctionGroupId(uint32);

	void OnListDblClick();
	void OnNewClick();
	void OnRenameClick();
	void OnDeleteClick();
	void OnImportClick();

	Framework::Win32::CListView* m_pList = nullptr;
	Framework::Win32::CButton* m_pNew = nullptr;
	Framework::Win32::CButton* m_pRename = nullptr;
	Framework::Win32::CButton* m_pDelete = nullptr;
	Framework::Win32::CButton* m_pImport = nullptr;

	Framework::FlatLayoutPtr m_pLayout;

	boost::signals2::connection m_functionTagsChangeConnection;

	CMIPS* m_context = nullptr;
	BiosDebugModuleInfoArray m_modules;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider = nullptr;
};
