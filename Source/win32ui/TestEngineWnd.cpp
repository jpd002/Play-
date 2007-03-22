#include "TestEngineWnd.h"
#include "win32/Rect.h"
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include "string_cast.h"
#include "../PS2VM.h"

#define CLSNAME _T("TestEngineWnd")

using namespace Framework;
using namespace boost;
using namespace std;

CTestEngineWnd::CTestEngineWnd(HWND hParent) :
m_pInstructionList(NULL),
m_pTestAllButton(NULL)
{
    if(!DoesWindowClassExist(CLSNAME))
    {
        WNDCLASSEX wc;
        memset(&wc, 0, sizeof(WNDCLASSEX));
        wc.cbSize			= sizeof(WNDCLASSEX);
        wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW); 
        wc.hInstance		= GetModuleHandle(NULL);
        wc.lpszClassName	= CLSNAME;
        wc.lpfnWndProc		= CWindow::WndProc;
        RegisterClassEx(&wc);
    }

    Create(NULL, CLSNAME, _T("Instruction Test Console"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, Win32::CRect(0, 0, 500, 300), hParent, NULL);
    SetClassPtr();

    m_pInstructionList = new Win32::CColumnTreeView(m_hWnd, Win32::CRect(0, 0, 1, 1), TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_FULLROWSELECT);
	m_pInstructionList->GetHeader()->InsertItem(_T("Instruction/Test case"), 100);
	m_pInstructionList->GetHeader()->InsertItem(_T("Succeded"), 100);
	m_pInstructionList->GetHeader()->InsertItem(_T("Failed"), 100);
    m_pInstructionList->GetHeader()->InsertItem(_T("Error"), 100);

    m_pTestAllButton = new Win32::CButton(_T("Test All"), m_hWnd, Win32::CRect(0, 0, 1, 1));

    m_Layout.InsertObject(new Win32::CLayoutWindow(1, 1, 1, 1, m_pInstructionList));

    {
        CHorizontalLayout* pTempLayout(new CHorizontalLayout());
        pTempLayout->InsertObject(Win32::CLayoutWindow::CreateButtonBehavior(130, 23, m_pTestAllButton));
        pTempLayout->InsertObject(new CLayoutStretch());
        m_Layout.InsertObject(pTempLayout);
    }

    RefreshInstructionMap();
    RefreshInstructionList();
    RefreshLayout();
}

CTestEngineWnd::~CTestEngineWnd()
{

}

long CTestEngineWnd::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
    RefreshLayout();
    return TRUE;
}

long CTestEngineWnd::OnCommand(unsigned short nCmd, unsigned short nId, HWND hWnd)
{
    if((m_pTestAllButton != NULL) && (m_pTestAllButton->m_hWnd == hWnd))
    {
        TestAll();
    }
    return TRUE;
}

long CTestEngineWnd::OnNotify(WPARAM wParam, NMHDR* pHdr)
{
    if((m_pInstructionList != NULL) && (pHdr->hwndFrom == m_pInstructionList->m_hWnd))
    {
        Win32::CColumnTreeView::MESSAGE* pMsg(
            static_cast<Win32::CColumnTreeView::MESSAGE*>(pHdr));
        
        switch(pMsg->code)
        {
        case NM_DBLCLK:
            OnItemDblClick();
            break;
        }
    }
    return FALSE;
}

void CTestEngineWnd::TestAll()
{
    for(InstructionByIdMapType::iterator itInstruction(m_InstructionsById.begin());
        itInstruction != m_InstructionsById.end(); itInstruction++)
    {
        Test(itInstruction->second);
    }
}

void CTestEngineWnd::Test(INSTRUCTION& Instruction)
{
    try
    {
        CMipsTestEngine Engine(("./tests/" + Instruction.sName + ".xml").c_str());

        unsigned int nFailed, nSuccess, nError, nTotal;

        nFailed = nSuccess = nError = nTotal = 0;

        for(CMipsTestEngine::OutputsType::iterator itOutput(Engine.GetOutputsBegin());
            itOutput != Engine.GetOutputsEnd(); itOutput++)
        {
            bool nHasSucceeded, nHasFailed, nHasError;

            nHasSucceeded = nHasFailed = nHasError = false;

            nTotal++;

            try
            {
                CMipsTestEngine::CValueSet* pInput(Engine.GetInput(itOutput->GetInputId()));
                CMipsTestEngine::CInstance* pInstance(Engine.GetInstance(itOutput->GetInstanceId()));

                if((pInput != NULL) && (pInstance != NULL))
                {
                    unsigned int nProgramSize;
                    const unsigned int nBaseAddress = 0x00100000;

                    nProgramSize = AssembleTestCase(nBaseAddress, pInput, pInstance);

                    CPS2VM::m_EE.m_State.nPC = nBaseAddress;
                    CPS2VM::m_EE.Execute(nProgramSize);

                    if(!itOutput->Verify(CPS2VM::m_EE))
                    {
                        nHasFailed = true;
                    }
                    else
                    {
                        nHasSucceeded = true;
                    }
                }
                else
                {
                    nHasError = true;
                }
            }
            catch(...)
            {
                nHasError = true;
            }

            if(nHasSucceeded) nSuccess++;
            if(nHasFailed) nFailed++;
            if(nHasError) nError++;

            //Insert the item into the list
            tstring sCaption;
            sCaption  = lexical_cast<tstring>(itOutput->GetInputId()) + _T("_");
            sCaption += lexical_cast<tstring>(itOutput->GetInstanceId());
            sCaption += tstring(_T("\t")) + (nHasSucceeded ? _T("X") : _T(""));
            sCaption += tstring(_T("\t")) + (nHasFailed ? _T("X") : _T(""));
            sCaption += tstring(_T("\t")) + (nHasError ? _T("X") : _T(""));

            HTREEITEM nCaseTreeItem;
            nCaseTreeItem = m_pInstructionList->GetTreeView()->InsertItem(Instruction.nTreeItem, sCaption.c_str());
            Instruction.TestCases[nCaseTreeItem] = INSTRUCTION::TestCaseType(itOutput->GetInputId(), itOutput->GetInstanceId());
        }

        //Update the totals
        tstring sCaption;
        sCaption  = string_cast<tstring>(Instruction.sName);
        sCaption += _T("\t") + lexical_cast<tstring>(nSuccess);
        sCaption += _T("\t") + lexical_cast<tstring>(nFailed);
        sCaption += _T("\t") + lexical_cast<tstring>(nError);

        m_pInstructionList->GetTreeView()->SetItemText(Instruction.nTreeItem, sCaption.c_str());
    }
    catch(const exception& Exception)
    {
        MessageBox(m_hWnd, 
            (_T("Error occured while parsing the test suite for instruction '") + 
            string_cast<tstring>(Instruction.sName) + _T("': \r\n\r\n") + 
            string_cast<tstring>(Exception.what())).c_str(),
            NULL,
            16);
    }

    m_pInstructionList->GetTreeView()->Redraw();
}

unsigned int CTestEngineWnd::AssembleTestCase(unsigned int nBaseAddress, CMipsTestEngine::CValueSet* pInput, CMipsTestEngine::CInstance* pInstance)
{
    CMIPSAssembler Assembler(reinterpret_cast<uint32*>(&CPS2VM::m_pRAM[nBaseAddress]));

    CPS2VM::m_EE.m_pExecMap->InvalidateBlocks();

    pInput->AssembleLoad(Assembler);
    Assembler.AssembleString(pInstance->GetSource());

    return Assembler.GetProgramSize();
}

void CTestEngineWnd::RefreshLayout()
{
    RECT rc(GetClientRect());

    SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

    m_Layout.SetRect(rc.left, rc.top, rc.right, rc.bottom);
    m_Layout.RefreshGeometry();

    Redraw();

    m_pInstructionList->GetHeader()->SetItemWidth(0, 0.5);

    m_pInstructionList->GetHeader()->SetItemWidth(1, 0.5 * 0.33);
    m_pInstructionList->GetHeader()->SetItemWidth(2, 0.5 * 0.33);
    m_pInstructionList->GetHeader()->SetItemWidth(3, 0.5 * 0.33);
}

void CTestEngineWnd::RefreshInstructionMap()
{
    filesystem::path TestsPath("./tests");

    filesystem::directory_iterator itEnd;
    for(filesystem::directory_iterator itFile(TestsPath);
        itFile != itEnd; itFile++)
    {
        if(filesystem::is_directory(*itFile)) continue;

        string sPath(itFile->leaf().c_str());
        string sExtension(sPath.end() - 3, sPath.end());

        if(_stricmp(sExtension.c_str(), "xml")) continue;

        INSTRUCTION Instruction;
        Instruction.sName       = string(sPath.begin(), sPath.end() - 4);
        Instruction.nTreeItem   = NULL;

        m_InstructionsById[static_cast<unsigned int>(m_InstructionsById.size())] = Instruction;
    }
}

void CTestEngineWnd::RefreshInstructionList()
{
    Win32::CTreeView* pTreeView(m_pInstructionList->GetTreeView());

    for(InstructionByIdMapType::iterator itInstruction(m_InstructionsById.begin());
        itInstruction != m_InstructionsById.end(); itInstruction++)
    {
        tstring sCaption;
        INSTRUCTION& Instruction(itInstruction->second);
        sCaption = string_cast<tstring>(Instruction.sName) + _T("\t?\t?\t?");
        Instruction.nTreeItem = pTreeView->InsertItem(TVI_ROOT, sCaption.c_str());
        pTreeView->SetItemParam(Instruction.nTreeItem, itInstruction->first);
    }
}

void CTestEngineWnd::OnItemDblClick()
{
    Win32::CTreeView* pTreeView(m_pInstructionList->GetTreeView());
    HTREEITEM nItem(pTreeView->GetSelection());

    if(nItem == NULL) return;

    HTREEITEM nParent(pTreeView->GetItemParent(nItem));

    if(nParent != NULL)
    {
        //This is a test case
        unsigned int nInstructionId(pTreeView->GetItemParam<unsigned int>(nParent));
        InstructionByIdMapType::iterator itInstruction(m_InstructionsById.find(nInstructionId));

        if(itInstruction == m_InstructionsById.end()) return;

        const INSTRUCTION& Instruction(itInstruction->second);
        INSTRUCTION::CaseListType::const_iterator itTestCase(Instruction.TestCases.find(nItem));

        if(itTestCase == Instruction.TestCases.end()) return;
        
        const INSTRUCTION::TestCaseType& TestCase(itTestCase->second);

        try
        {
            CMipsTestEngine Engine(("./tests/" + Instruction.sName + ".xml").c_str());
            CMipsTestEngine::CValueSet* pInput(Engine.GetInput(TestCase.first));
            CMipsTestEngine::CInstance* pInstance(Engine.GetInstance(TestCase.second));

            if((pInput != NULL) && (pInstance != NULL))
            {
                const unsigned int nBaseAddress = 0x00100000;
                AssembleTestCase(nBaseAddress, pInput, pInstance);
                CPS2VM::m_EE.m_State.nPC = nBaseAddress;
                m_OnTestCaseLoad(nBaseAddress);
            }
        }
        catch(const exception& Exception)
        {
            MessageBox(m_hWnd, 
                (_T("Error occured while trying to assemble the test case: \r\n\r\n") + 
                string_cast<tstring>(Exception.what())).c_str(),
                NULL,
                16);
        }
    }
    else
    {
        //Instruction
    }
}
