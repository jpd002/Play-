#include <stdio.h>
#include <string.h>
#include "RegViewFPU.h"
#include "PS2VM.h"

using namespace Framework;

CRegViewFPU::CRegViewFPU(HWND hParent, RECT* pR, CMIPS* pC) :
CRegViewPage(hParent, pR)
{
	m_nViewMode = VIEWMODE_SINGLE;

	m_pCtx = pC;
	
	m_pOnMachineStateChangeHandler = new CEventHandlerMethod<CRegViewFPU, int>(this, &CRegViewFPU::OnMachineStateChange);
	m_pOnRunningStateChangeHandler = new CEventHandlerMethod<CRegViewFPU, int>(this, &CRegViewFPU::OnRunningStateChange);

	CPS2VM::m_OnMachineStateChange.InsertHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.InsertHandler(m_pOnRunningStateChangeHandler);
}

CRegViewFPU::~CRegViewFPU()
{
	CPS2VM::m_OnMachineStateChange.RemoveHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.RemoveHandler(m_pOnRunningStateChangeHandler);
}

void CRegViewFPU::Update()
{
	CStrA sText;
	GetDisplayText(&sText);
	SetDisplayText(sText);
	CRegViewPage::Update();
}

void CRegViewFPU::GetDisplayText(CStrA* pText)
{
	switch(m_nViewMode)
	{
	case VIEWMODE_WORD:
		RenderWord(pText);
		break;
	case VIEWMODE_SINGLE:
		RenderSingle(pText);
		break;
	}
}

void CRegViewFPU::RenderFCSR(CStrA* pText)
{
	char sText[256];

	sprintf(sText, "FCSR: 0x%0.8X\r\n", m_pCtx->m_State.nFCSR);
	(*pText) += sText;

	sprintf(sText, "CC  : %i%i%i%i%i%i%i%ib\r\n", \
		(m_pCtx->m_State.nFCSR & 0x80000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x40000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x20000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x10000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x04000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x08000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x02000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x00800000) != 0 ? 1 : 0);

	(*pText) += sText;
}

void CRegViewFPU::RenderWord(CStrA* pText)
{
	char sText[256];
	char sReg[256];
	MIPSSTATE* s;
	uint32 nData;
	unsigned int i;

	s = &m_pCtx->m_State;

	for(i = 0; i < 32; i++)
	{
		if(i < 10)
		{
			sprintf(sReg, "F%i  ", i);
		}
		else
		{
			sprintf(sReg, "F%i ", i);
		}

		nData = ((uint32*)s->nCOP10)[i * 2];

		sprintf(sText, "%s: 0x%0.8X\r\n", sReg, nData);
		(*pText) += sText;
	}

	RenderFCSR(pText);
}

void CRegViewFPU::RenderSingle(CStrA* pText)
{
	char sText[256];
	char sReg[256];
	MIPSSTATE* s;
	float nValue;
	uint32 nData;
	unsigned int i;

	s = &m_pCtx->m_State;

	for(i = 0; i < 32; i++)
	{
		if(i < 10)
		{
			sprintf(sReg, "F%i  ", i);
		}
		else
		{
			sprintf(sReg, "F%i ", i);
		}

		nData = ((uint32*)s->nCOP10)[i * 2];
		nValue = *(float*)(&nData);

		sprintf(sText, "%s: %+.24e\r\n", sReg, nValue);
	
		(*pText) += sText;
	}
	
	nValue = *(float*)(&s->nCOP1A);
	sprintf(sText, "ACC : %+.24e\r\n", nValue);
	(*pText) += sText;

	RenderFCSR(pText);
}

long CRegViewFPU::OnRightButtonUp(unsigned int nX, unsigned int nY)
{
	POINT pt;
	HMENU hMenu;

	pt.x = nX;
	pt.y = nY;
	ClientToScreen(m_hWnd, &pt);

	hMenu = CreatePopupMenu();
	InsertMenu(hMenu, 0, MF_BYPOSITION | (m_nViewMode == 0 ? MF_CHECKED : 0), 40000 + 0, _X("32 Bits Integers"));
	InsertMenu(hMenu, 1, MF_BYPOSITION | (m_nViewMode == 1 ? MF_CHECKED : 0), 40000 + 1, _X("64 Bits Integers"));
	InsertMenu(hMenu, 2, MF_BYPOSITION | (m_nViewMode == 2 ? MF_CHECKED : 0), 40000 + 2, _X("Single Precision Floating-Point Numbers"));
	InsertMenu(hMenu, 3, MF_BYPOSITION | (m_nViewMode == 3 ? MF_CHECKED : 0), 40000 + 3, _X("Double Precision Floating-Point Numbers"));

	TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_hWnd, NULL); 

	return FALSE;
}

long CRegViewFPU::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	if((nID >= 40000) && (nID < (40000 + VIEWMODE_MAX)))
	{
		m_nViewMode = (VIEWMODE)(nID - 40000);
		Update();
	}

	return TRUE;
}

void CRegViewFPU::OnRunningStateChange(int nNothing)
{
	Update();
}

void CRegViewFPU::OnMachineStateChange(int nNothing)
{
	Update();
}
