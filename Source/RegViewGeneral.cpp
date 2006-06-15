#include <stdio.h>
#include <string.h>
#include "RegViewGeneral.h"
#include "PS2VM.h"

using namespace Framework;

CRegViewGeneral::CRegViewGeneral(HWND hParent, RECT* pR, CMIPS* pC) :
CRegViewPage(hParent, pR)
{
	m_pCtx = pC;
	
	m_pOnMachineStateChangeHandler = new CEventHandlerMethod<CRegViewGeneral, int>(this, &CRegViewGeneral::OnMachineStateChange);
	m_pOnRunningStateChangeHandler = new CEventHandlerMethod<CRegViewGeneral, int>(this, &CRegViewGeneral::OnRunningStateChange);

	CPS2VM::m_OnMachineStateChange.InsertHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.InsertHandler(m_pOnRunningStateChangeHandler);
}

CRegViewGeneral::~CRegViewGeneral()
{
	CPS2VM::m_OnMachineStateChange.RemoveHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.RemoveHandler(m_pOnRunningStateChangeHandler);
}

void CRegViewGeneral::Update()
{
	CStrA sText;
	GetDisplayText(&sText);
	SetDisplayText(sText);
	CRegViewPage::Update();
}

void CRegViewGeneral::GetDisplayText(CStrA* pText)
{
	char sTemp[256];
	MIPSSTATE* s;
	unsigned int i;

	s = &m_pCtx->m_State;

	for(i = 0; i < 32; i++)
	{
		sprintf(sTemp, "%s : 0x%0.8X%0.8X%0.8X%0.8X\r\n", CMIPS::m_sGPRName[i], s->nGPR[i].nV[3], s->nGPR[i].nV[2], s->nGPR[i].nV[1], s->nGPR[i].nV[0]);
		(*pText) += sTemp;
	}

	sprintf(sTemp, "LO : 0x%0.8X%0.8X\r\n", s->nLO[1], s->nLO[0]);
	(*pText) += sTemp;

	sprintf(sTemp, "HI : 0x%0.8X%0.8X\r\n", s->nHI[1], s->nHI[0]);
	(*pText) += sTemp;

	sprintf(sTemp, "LO1: 0x%0.8X%0.8X\r\n", s->nLO1[1], s->nLO1[0]);
	(*pText) += sTemp;

	sprintf(sTemp, "HI1: 0x%0.8X%0.8X\r\n", s->nHI1[1], s->nHI1[0]);
	(*pText) += sTemp;

	sprintf(sTemp, "SA : 0x%0.8X\r\n", s->nSA);
	(*pText) += sTemp;
}

void CRegViewGeneral::OnRunningStateChange(int nNothing)
{
	Update();
}

void CRegViewGeneral::OnMachineStateChange(int nNothing)
{
	Update();
}
