#ifndef _TESTENGINEWND_H_
#define _TESTENGINEWND_H_

#include "DebuggerChildWnd.h"
#include "win32/ColumnTreeView.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include <map>

class CTestEngineWnd : public CDebuggerChildWnd
{
public:
                                            CTestEngineWnd(HWND);
    virtual                                 ~CTestEngineWnd();

protected:
    long                                    OnSize(unsigned int, unsigned int, unsigned int);
    long                                    OnCommand(unsigned short, unsigned short, HWND);

private:
    struct TESTCASE
    {
        unsigned int nInputId;
        unsigned int nInstanceId;
    };

    struct INSTRUCTION
    {
        //typedef std::vector<TESTCASE> CaseListType;

        std::string     sName;
        HTREEITEM       nTreeItem;
        //CaseListType    TestCases;
    };

    typedef std::map<unsigned int, INSTRUCTION> InstructionByIdMapType;

    void                                    TestAll();
    void                                    Test(const INSTRUCTION&);
    void                                    RefreshLayout();
    void                                    RefreshInstructionMap();
    void                                    RefreshInstructionList();

    Framework::Win32::CColumnTreeView*      m_pInstructionList;
    Framework::Win32::CButton*              m_pTestAllButton;
    Framework::CVerticalLayout              m_Layout;
    InstructionByIdMapType                  m_InstructionsById;
};

#endif
