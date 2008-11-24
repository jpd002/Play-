#include <stdio.h>
#include "ELFView.h"
#include "PtrMacro.h"
#include <boost/lexical_cast.hpp>
#include "string_cast.h"

using namespace Framework;
using namespace std;
using namespace boost;

CELFView::CELFView(HWND hParent) :
COptionWnd<CMDIChild>(hParent, _T("ELF File Viewer")),
m_pELF(NULL),
m_pHeaderView(NULL),
m_pSymbolView(NULL),
m_pSectionView(NULL),
m_pProgramView(NULL)
{

}

CELFView::~CELFView()
{
	Delete();
}

void CELFView::Delete()
{
	if(m_pELF != NULL)
	{
        const ELFHEADER& header = m_pELF->GetHeader();
		for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
		{
			DELETEPTR(m_pProgramView[i]);
		}
		DELETEPTR(m_pProgramView);

		for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
		{
			DELETEPTR(m_pSectionView[i]);
		}
		DELETEPTR(m_pSectionView);

		DELETEPTR(m_pSymbolView);
		DELETEPTR(m_pHeaderView);
	}
}

void CELFView::SetELF(CELF* pELF)
{
	DeleteAllOptions();

	Delete();

	m_pELF = pELF;

	if(m_pELF == NULL) return;

	HWND hCont = GetContainer()->m_hWnd;

	m_pHeaderView = new CELFHeaderView(hCont, m_pELF);
	m_pSymbolView = new CELFSymbolView(hCont, m_pELF);

    const ELFHEADER& header(m_pELF->GetHeader());

	m_pSectionView = new CELFSectionView*[header.nSectHeaderCount];
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
		m_pSectionView[i] = new CELFSectionView(hCont, m_pELF, i);
	}

	m_pProgramView = new CELFProgramView*[header.nProgHeaderCount];
	for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
	{
		m_pProgramView[i] = new CELFProgramView(hCont, m_pELF, i);
	}

	PopulateList();

	GetTreeView()->SetSelection(GetTreeView()->GetRoot());

	//Humm, this window is not necessarily active at this moment? Why this is here?
	//GetTreeView()->SetFocus();

}

void CELFView::PopulateList()
{
    InsertOption(NULL, _T("Header"), m_pHeaderView->m_hWnd);
	HTREEITEM hItem = InsertOption(NULL, _T("Sections"), NULL);
    const ELFHEADER& header = m_pELF->GetHeader();

	const char* sStrTab = (const char*)m_pELF->GetSectionData(header.nSectHeaderStringTableIndex);
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
        tstring sDisplay;
    	const char* sName;

    	ELFSECTIONHEADER* pSect = m_pELF->GetSection(i);
		
        if(sStrTab != NULL)
		{
			sName = sStrTab + pSect->nStringTableIndex;
		}
		else
		{
			sName = "";
		}

        if(strlen(sName))
		{
            sDisplay = string_cast<tstring>(sName);
		}
		else
		{
            sDisplay = _T("Section ") + lexical_cast<tstring>(i);
		}

        InsertOption(hItem, sDisplay.c_str(), m_pSectionView[i]->m_hWnd);
	}
	GetTreeView()->Expand(hItem, TVE_EXPAND);

	if(header.nProgHeaderCount != 0)
	{
		hItem = InsertOption(NULL, _T("Segments"), NULL);

		for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
		{
            tstring sDisplay(_T("Segment ") + lexical_cast<tstring>(i));
			InsertOption(hItem, sDisplay.c_str(), m_pProgramView[i]->m_hWnd);
		}
		GetTreeView()->Expand(hItem, TVE_EXPAND);
	
	}

	if(m_pELF->FindSection(".strtab") && m_pELF->FindSection(".symtab"))
	{
		InsertOption(NULL, _T("Symbols"), m_pSymbolView->m_hWnd);
	}

}

long CELFView::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}
