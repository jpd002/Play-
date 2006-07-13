#include <stdio.h>
#include "ELFView.h"
#include "PtrMacro.h"

using namespace Framework;

CELFView::CELFView(HWND hParent) :
COptionWnd<CMDIChild>(hParent, _X("ELF File Viewer"))
{
	m_pELF = NULL;
}

CELFView::~CELFView()
{
	Delete();
}

void CELFView::Delete()
{
	unsigned int i;

	if(m_pELF != NULL)
	{
		for(i = 0; i < m_pELF->m_Header.nProgHeaderCount; i++)
		{
			DELETEPTR(m_pProgramView[i]);
		}
		DELETEPTR(m_pProgramView);

		for(i = 0; i < m_pELF->m_Header.nSectHeaderCount; i++)
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
	HWND hCont;
	unsigned int i;

	DeleteAllOptions();

	Delete();

	m_pELF = pELF;

	if(m_pELF == NULL) return;

	hCont = GetContainer()->m_hWnd;

	m_pHeaderView = new CELFHeaderView(hCont, m_pELF);
	m_pSymbolView = new CELFSymbolView(hCont, m_pELF);

	m_pSectionView = new CELFSectionView*[m_pELF->m_Header.nSectHeaderCount];
	for(i = 0; i < m_pELF->m_Header.nSectHeaderCount; i++)
	{
		m_pSectionView[i] = new CELFSectionView(hCont, m_pELF, i);
	}

	m_pProgramView = new CELFProgramView*[m_pELF->m_Header.nProgHeaderCount];
	for(i = 0; i < m_pELF->m_Header.nProgHeaderCount; i++)
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
	ELFSECTIONHEADER* pSect;
	HTREEITEM hItem;
	const char* sName;
	const char* sStrTab;
	xchar sDisplay[256];
	unsigned int i;

	InsertOption(NULL, _X("Header"), m_pHeaderView->m_hWnd);
	hItem = InsertOption(NULL, _X("Sections"), NULL);

	sStrTab = (const char*)m_pELF->GetSectionData(m_pELF->m_Header.nSectHeaderStringTableIndex);
	for(i = 0; i < m_pELF->m_Header.nSectHeaderCount; i++)
	{
		pSect = m_pELF->GetSection(i);
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
			xconvert(sDisplay, sName, 256);
		}
		else
		{
			xsnprintf(sDisplay, countof(sDisplay), _X("Section %i"), i);
		}
		InsertOption(hItem, sDisplay, m_pSectionView[i]->m_hWnd);
	}
	GetTreeView()->Expand(hItem, TVE_EXPAND);

	if(m_pELF->m_Header.nProgHeaderCount != 0)
	{
		hItem = InsertOption(NULL, _X("Segments"), NULL);

		for(i = 0; i < m_pELF->m_Header.nProgHeaderCount; i++)
		{
			xsnprintf(sDisplay, countof(sDisplay), _X("Segment %i"), i);
			InsertOption(hItem, sDisplay, m_pProgramView[i]->m_hWnd);
		}
		GetTreeView()->Expand(hItem, TVE_EXPAND);
	
	}

	if(m_pELF->FindSection(".strtab") && m_pELF->FindSection(".symtab"))
	{
		InsertOption(NULL, _X("Symbols"), m_pSymbolView->m_hWnd);
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
