#ifndef _TESTENGINEWND_H_
#define _TESTENGINEWND_H_

#include "DebuggerChildWnd.h"
#include "win32/ColumnTreeView.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include <map>
#include <vector>

class CTestEngineWnd : public CDebuggerChildWnd
{
public:
                                            CTestEngineWnd(HWND);
    virtual                                 ~CTestEngineWnd();

protected:
    long                                    OnSize(unsigned int, unsigned int, unsigned int);
    long                                    OnCommand(unsigned short, unsigned short, HWND);
    long                                    OnNotify(WPARAM, NMHDR*);

private:
    struct ITEM
    {
        virtual ~ITEM() { }
    };

    struct TESTCASE : public ITEM
    {
        unsigned int nInputId;
        unsigned int nInstanceId;
    };

    struct INSTRUCTION : public ITEM
    {
        typedef std::map<HTREEITEM, TESTCASE> CaseListType;

        std::string     sName;
        HTREEITEM       nTreeItem;
        CaseListType    TestCases;
    };

    typedef std::map<unsigned int, INSTRUCTION> InstructionByIdMapType;

    void                                    TestAll();
    void                                    Test(INSTRUCTION&);
    void                                    RefreshLayout();
    void                                    RefreshInstructionMap();
    void                                    RefreshInstructionList();
    void                                    OnItemDblClick();

    Framework::Win32::CColumnTreeView*      m_pInstructionList;
    Framework::Win32::CButton*              m_pTestAllButton;
    Framework::CVerticalLayout              m_Layout;
    InstructionByIdMapType                  m_InstructionsById;
};

#endif
