#ifndef _TESTENGINEWND_H_
#define _TESTENGINEWND_H_

#include "DebuggerChildWnd.h"
#include "win32/ColumnTreeView.h"
#include "win32/Button.h"
#include "win32/Edit.h"
#include "win32/Layouts.h"
#include "../MipsTestEngine.h"
#include <map>
#include <boost/signal.hpp>

class CTestEngineWnd : public CDebuggerChildWnd
{
public:
                                            CTestEngineWnd(HWND);
    virtual                                 ~CTestEngineWnd();

    boost::signal<void (uint32)>            m_OnTestCaseLoad;

protected:
    long                                    OnSize(unsigned int, unsigned int, unsigned int);
    long                                    OnCommand(unsigned short, unsigned short, HWND);
    long                                    OnNotify(WPARAM, NMHDR*);

private:
    struct INSTRUCTION
    {
        typedef std::pair<unsigned int, unsigned int> TestCaseType;
        typedef std::map<HTREEITEM, TestCaseType> CaseListType;

        std::string     sName;
        HTREEITEM       nTreeItem;
        CaseListType    TestCases;
    };

    typedef std::map<unsigned int, INSTRUCTION> InstructionByIdMapType;
    typedef boost::function<void (const INSTRUCTION*, const INSTRUCTION::TestCaseType*)> ListMessageHandlerType;

    void                                    TestAll();
    void                                    Test(INSTRUCTION&);
    unsigned int                            AssembleTestCase(unsigned int, CMipsTestEngine::CValueSet*, CMipsTestEngine::CInstance*);
    void                                    RefreshLayout();
    void                                    RefreshInstructionMap();
    void                                    RefreshInstructionList();
    void                                    ProcessListEvent(ListMessageHandlerType);
    void                                    OnItemDblClick(const INSTRUCTION*, const INSTRUCTION::TestCaseType*);
    void                                    OnListSelChange(const INSTRUCTION*, const INSTRUCTION::TestCaseType*);

    Framework::Win32::CColumnTreeView*      m_pInstructionList;
    Framework::Win32::CEdit*                m_pDescriptionEdit;
    Framework::Win32::CButton*              m_pTestAllButton;
    Framework::CVerticalLayout              m_Layout;
    InstructionByIdMapType                  m_InstructionsById;
};

#endif
