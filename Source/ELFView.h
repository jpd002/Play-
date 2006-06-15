#ifndef _ELFVIEW_H_
#define _ELFVIEW_H_

#include "OptionWnd.h"
#include "ELF.h"
#include "ELFHeaderView.h"
#include "ELFSectionView.h"
#include "ELFProgramView.h"
#include "ELFSymbolView.h"
#include "win32/MDIChild.h"

class CELFView : public COptionWnd<Framework::CMDIChild>
{
public:
						CELFView(HWND);
						~CELFView();
	void				SetELF(CELF*);
protected:
	long				OnSysCommand(unsigned int, LPARAM);

private:
	void				PopulateList();
	void				Delete();
	CELF*				m_pELF;
	CELFHeaderView*		m_pHeaderView;
	CELFSymbolView*		m_pSymbolView;
	CELFSectionView**	m_pSectionView;
	CELFProgramView**	m_pProgramView;
};

#endif