#ifndef _FUNCTIONSVIEW_H_
#define _FUNCTIONSVIEW_H_

#include <boost/signal.hpp>
#include <functional>
#include "win32/MDIChild.h"
#include "win32/ListView.h"
#include "win32/Button.h"
#include "layout/VerticalLayout.h"
#include "../MIPS.h"
#include "../ELF.h"

class CFunctionsView : public Framework::Win32::CMDIChild, boost::signals::trackable
{
public:
    struct MODULE
    {
        std::string     name;
        uint32          begin;
        uint32          end;
    };

    typedef std::list<MODULE> ModuleList;
    typedef std::tr1::function<ModuleList ()> ModuleListProvider;


									CFunctionsView(HWND);
	virtual							~CFunctionsView();

    void                            SetContext(CMIPS*, const ModuleListProvider&);
	void							SetELF(CELF*);

	void							Refresh();

	boost::signal<void (uint32)>	m_OnFunctionDblClick;
	boost::signal<void (void)>		m_OnFunctionsStateChange;

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnNotify(WPARAM, NMHDR*);
	long							OnSysCommand(unsigned int, LPARAM);

private:
	void							CreateListColumns();
	void							ResizeListColumns();
	void							RefreshLayout();
	void							RefreshList();
    void                            InitializeModuleGrouper();
    uint32                          GetFunctionGroupId(uint32);

	void							OnListDblClick();
	void							OnNewClick();
	void							OnRenameClick();
	void							OnDeleteClick();
	void							OnImportClick();

	Framework::Win32::CListView*	m_pList;
	Framework::Win32::CButton*		m_pNew;
	Framework::Win32::CButton*		m_pRename;
	Framework::Win32::CButton*		m_pDelete;
	Framework::Win32::CButton*		m_pImport;

    Framework::FlatLayoutPtr        m_pLayout;

    boost::signals::connection      m_functionTagsChangeConnection;

    CMIPS*							m_pCtx;
	CELF*							m_pELF;
    ModuleList                      m_modules;
    ModuleListProvider              m_moduleListProvider;
};

#endif
