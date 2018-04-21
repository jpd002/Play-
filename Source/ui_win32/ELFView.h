#pragma once

#include "../ELF.h"
#include "ELFHeaderView.h"
#include "ELFProgramView.h"
#include "ELFSectionView.h"
#include "ELFSymbolView.h"
#include "OptionWnd.h"
#include "win32/MDIChild.h"
#include <unordered_map>

class CELFView : public COptionWnd<Framework::Win32::CMDIChild>
{
public:
	CELFView(HWND);
	virtual ~CELFView();

	void SetELF(CELF*);

protected:
	long OnSysCommand(unsigned int, LPARAM) override;

	void OnItemAppearing(HTREEITEM) override;

private:
	typedef std::unordered_map<HTREEITEM, int> SectionItemMap;

	void PopulateList();
	void Delete();

	CELF*            m_pELF;
	CELFHeaderView*  m_pHeaderView;
	CELFSymbolView*  m_pSymbolView;
	CELFSectionView* m_pSectionView;
	CELFProgramView* m_pProgramView;

	SectionItemMap m_sectionItems;
	SectionItemMap m_programItems;
};
