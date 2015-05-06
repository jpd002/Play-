#include <stdio.h>
#include "ELFView.h"
#include "PtrMacro.h"
#include <boost/lexical_cast.hpp>
#include "string_cast.h"

using namespace Framework;

CELFView::CELFView(HWND hParent) 
: COptionWnd<CMDIChild>(hParent, _T("ELF File Viewer"))
, m_pELF(NULL)
, m_pHeaderView(NULL)
, m_pSymbolView(NULL)
, m_pSectionView(NULL)
, m_pProgramView(NULL)
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
		DELETEPTR(m_pProgramView);
		DELETEPTR(m_pSectionView);
		DELETEPTR(m_pSymbolView);
		DELETEPTR(m_pHeaderView);
	}

	m_sectionItems.clear();
	m_programItems.clear();
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

	m_pSectionView = new CELFSectionView(hCont, m_pELF);
	m_pProgramView = new CELFProgramView(hCont, m_pELF);

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
		std::tstring sDisplay;
    	const char* sName(NULL);

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
			sDisplay = string_cast<std::tstring>(sName);
		}
		else
		{
			sDisplay = _T("Section ") + boost::lexical_cast<std::tstring>(i);
		}

        HTREEITEM sectionItem = InsertOption(hItem, sDisplay.c_str(), m_pSectionView->m_hWnd);
		m_sectionItems[sectionItem] = i;
	}
	GetTreeView()->Expand(hItem, TVE_EXPAND);

	if(header.nProgHeaderCount != 0)
	{
		hItem = InsertOption(NULL, _T("Segments"), NULL);

		for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
		{
			std::tstring sDisplay(_T("Segment ") + boost::lexical_cast<std::tstring>(i));
			HTREEITEM programItem = InsertOption(hItem, sDisplay.c_str(), m_pProgramView->m_hWnd);
			m_programItems[programItem] = i;
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

void CELFView::OnItemAppearing(HTREEITEM item)
{
	//Check if it's a section header
	{
		SectionItemMap::const_iterator sectionIterator = m_sectionItems.find(item);
		if(sectionIterator != m_sectionItems.end())
		{
			m_pSectionView->SetSectionIndex(static_cast<uint16>(sectionIterator->second));
			return;
		}
	}

	//Check if it's a program header
	{
		SectionItemMap::const_iterator programIterator = m_programItems.find(item);
		if(programIterator != m_programItems.end())
		{
			m_pProgramView->SetProgramIndex(static_cast<uint16>(programIterator->second));
			return;
		}
	}
}
