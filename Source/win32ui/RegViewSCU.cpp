#include <stdio.h>
#include <string.h>
#include "RegViewSCU.h"
#include "../COP_SCU.h"
#include "../PS2VM.h"

using namespace Framework;

CRegViewSCU::CRegViewSCU(HWND hParent, RECT* pR, CMIPS* pC) :
CRegViewPage(hParent, pR)
{
	m_pCtx = pC;
	
	m_pOnMachineStateChangeHandler = new CEventHandlerMethod<CRegViewSCU, int>(this, &CRegViewSCU::OnMachineStateChange);
	m_pOnRunningStateChangeHandler = new CEventHandlerMethod<CRegViewSCU, int>(this, &CRegViewSCU::OnRunningStateChange);

	CPS2VM::m_OnMachineStateChange.InsertHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.InsertHandler(m_pOnRunningStateChangeHandler);
}

CRegViewSCU::~CRegViewSCU()
{
	CPS2VM::m_OnMachineStateChange.RemoveHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.RemoveHandler(m_pOnRunningStateChangeHandler);
}

void CRegViewSCU::Update()
{
	CStrA sText;
	GetDisplayText(&sText);
	SetDisplayText(sText);
	CRegViewPage::Update();
}

void CRegViewSCU::GetDisplayText(CStrA* pText)
{
	char sTemp[256];
	MIPSSTATE* s;
	unsigned int i;

	s = &m_pCtx->m_State;

	for(i = 0; i < 32; i++)
	{
		sprintf(sTemp, "%10s : 0x%0.8X\r\n", CCOP_SCU::m_sRegName[i], s->nCOP0[i]);
		(*pText) += sTemp;
	}
}

void CRegViewSCU::OnRunningStateChange(int nNothing)
{
	Update();
}

void CRegViewSCU::OnMachineStateChange(int nNothing)
{
	Update();
}
