#include <stdio.h>
#include <string.h>
#include "RegViewFPU.h"

CRegViewFPU::CRegViewFPU(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* pC) 
: CRegViewPage(hParent, rect)
, m_pCtx(pC)
, m_nViewMode(VIEWMODE_SINGLE)
{
	virtualMachine.OnMachineStateChange.connect(boost::bind(&CRegViewFPU::OnMachineStateChange, this));
	virtualMachine.OnRunningStateChange.connect(boost::bind(&CRegViewFPU::OnRunningStateChange, this));
}

CRegViewFPU::~CRegViewFPU()
{

}

void CRegViewFPU::Update()
{
	SetDisplayText(GetDisplayText().c_str());
	CRegViewPage::Update();
}

std::string CRegViewFPU::GetDisplayText()
{
	switch(m_nViewMode)
	{
	case VIEWMODE_WORD:
		return RenderWord();
		break;
	case VIEWMODE_SINGLE:
		return RenderSingle();
		break;
	default:
		return std::string();
		break;
	}
}

std::string CRegViewFPU::RenderFCSR()
{
	char sText[256];
	std::string result;

	sprintf(sText, "FCSR: 0x%0.8X\r\n", m_pCtx->m_State.nFCSR);
	result += sText;

	sprintf(sText, "CC  : %i%i%i%i%i%i%i%ib\r\n", \
		(m_pCtx->m_State.nFCSR & 0x80000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x40000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x20000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x10000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x04000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x08000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x02000000) != 0 ? 1 : 0, \
		(m_pCtx->m_State.nFCSR & 0x00800000) != 0 ? 1 : 0);
	result += sText;

	return result;
}

std::string CRegViewFPU::RenderWord()
{
	std::string result;
	MIPSSTATE* s = &m_pCtx->m_State;

	for(unsigned int i = 0; i < 32; i++)
	{
		char sText[256];
		char sReg[256];

		if(i < 10)
		{
			sprintf(sReg, "F%i  ", i);
		}
		else
		{
			sprintf(sReg, "F%i ", i);
		}

		uint32 nData = ((uint32*)s->nCOP1)[i];

		sprintf(sText, "%s: 0x%0.8X\r\n", sReg, nData);
		result += sText;
	}

	result += RenderFCSR();
	return result;
}

std::string CRegViewFPU::RenderSingle()
{
	char sText[256];
	std::string result;
	MIPSSTATE* s = &m_pCtx->m_State;

	for(unsigned int i = 0; i < 32; i++)
	{
		char sReg[256];

		if(i < 10)
		{
			sprintf(sReg, "F%i  ", i);
		}
		else
		{
			sprintf(sReg, "F%i ", i);
		}

		uint32 nData = ((uint32*)s->nCOP1)[i];
		float nValue = *(float*)(&nData);

		sprintf(sText, "%s: %+.24e\r\n", sReg, nValue);
	
		result += sText;
	}
	
	float nValue = *(float*)(&s->nCOP1A);
	sprintf(sText, "ACC : %+.24e\r\n", nValue);
	result += sText;

	result += RenderFCSR();
	return result;
}

long CRegViewFPU::OnRightButtonUp(int nX, int nY)
{
	POINT pt = { nX, nY };
	ClientToScreen(m_hWnd, &pt);

	HMENU hMenu = CreatePopupMenu();
	InsertMenu(hMenu, 0, MF_BYPOSITION | (m_nViewMode == VIEWMODE_WORD ? MF_CHECKED : 0),	40000 + VIEWMODE_WORD,		_T("32 Bits Integers"));
	InsertMenu(hMenu, 1, MF_BYPOSITION | (m_nViewMode == VIEWMODE_SINGLE ? MF_CHECKED : 0),	40000 + VIEWMODE_SINGLE,	_T("Single Precision Floating-Point Numbers"));

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

void CRegViewFPU::OnRunningStateChange()
{
	Update();
}

void CRegViewFPU::OnMachineStateChange()
{
	Update();
}
