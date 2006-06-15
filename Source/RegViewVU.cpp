#include <stdio.h>
#include "RegViewVU.h"
#include "PS2VM.h"

using namespace Framework;

CRegViewVU::CRegViewVU(HWND hParent, RECT* pR, CMIPS* pCtx) :
CRegViewPage(hParent, pR)
{
	m_pCtx = pCtx;

	m_pOnMachineStateChangeHandler = new CEventHandlerMethod<CRegViewVU, int>(this, &CRegViewVU::OnMachineStateChange);
	m_pOnRunningStateChangeHandler = new CEventHandlerMethod<CRegViewVU, int>(this, &CRegViewVU::OnRunningStateChange);

	CPS2VM::m_OnMachineStateChange.InsertHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.InsertHandler(m_pOnRunningStateChangeHandler);

	Update();
}

CRegViewVU::~CRegViewVU()
{
	CPS2VM::m_OnMachineStateChange.RemoveHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.RemoveHandler(m_pOnRunningStateChangeHandler);
}

void CRegViewVU::Update()
{
	CStrA sText;
	GetDisplayText(&sText);
	SetDisplayText(sText);
	CRegViewPage::Update();
}

void CRegViewVU::GetDisplayText(CStrA* pText)
{
	char sLine[256];
	char sReg1[32];
	char sReg2[32];
	unsigned int i;
	MIPSSTATE* pState;

	(*pText) += "              x               y       \r\n";
	(*pText) += "              z               w       \r\n";

	pState = &m_pCtx->m_State;

	for(i = 0; i < 32; i++)
	{
		if(i < 10)
		{
			sprintf(sReg1, "VF%i  ", i);
		}
		else
		{
			sprintf(sReg1, "VF%i ", i);
		}

		sprintf(sLine, "%s: %+.7e %+.7e\r\n       %+.7e %+.7e\r\n", sReg1, \
			*(float*)&pState->nCOP2[i].nV0, \
			*(float*)&pState->nCOP2[i].nV1, \
			*(float*)&pState->nCOP2[i].nV2, \
			*(float*)&pState->nCOP2[i].nV3);

		(*pText) += sLine;
	}

	sprintf(sLine, "ACC  : %+.7e %+.7e\r\n       %+.7e %+.7e\r\n", \
		*(float*)&pState->nCOP2A.nV0, \
		*(float*)&pState->nCOP2A.nV1, \
		*(float*)&pState->nCOP2A.nV2, \
		*(float*)&pState->nCOP2A.nV3);

	(*pText) += sLine;

	sprintf(sLine, "Q    : %+.7e\r\n", *(float*)&pState->nCOP2Q);
	(*pText) += sLine;

	sprintf(sLine, "I    : %+.7e\r\n", *(float*)&pState->nCOP2I);
	(*pText) += sLine;

	sprintf(sLine, "P    : %+.7e\r\n", *(float*)&pState->nCOP2P);
	(*pText) += sLine;

	sprintf(sLine, "R    : %+.7e\r\n", *(float*)&pState->nCOP2R);
	(*pText) += sLine;

	sprintf(sLine, "MACSF: %i%i%i%ib\r\n", \
		(pState->nCOP2SF.nV0 != 0) ? 1 : 0, \
		(pState->nCOP2SF.nV1 != 0) ? 1 : 0, \
		(pState->nCOP2SF.nV2 != 0) ? 1 : 0, \
		(pState->nCOP2SF.nV3 != 0) ? 1 : 0);
	(*pText) += sLine;

	sprintf(sLine, "MACZF: %i%i%i%ib\r\n", \
		(pState->nCOP2ZF.nV0 != 0) ? 1 : 0, \
		(pState->nCOP2ZF.nV1 != 0) ? 1 : 0, \
		(pState->nCOP2ZF.nV2 != 0) ? 1 : 0, \
		(pState->nCOP2ZF.nV3 != 0) ? 1 : 0);
	(*pText) += sLine;

	sprintf(sLine, "CLIP : 0x%0.6X\r\n", pState->nCOP2CF);
	(*pText) += sLine;

	for(i = 0; i < 16; i += 2)
	{
		if(i < 10)
		{
			sprintf(sReg1, "VI%i  ", i);
			sprintf(sReg2, "VI%i  ", i + 1);
		}
		else
		{
			sprintf(sReg1, "VI%i ", i);
			sprintf(sReg2, "VI%i ", i + 1);
		}

		sprintf(sLine, "%s: 0x%0.4X    %s: 0x%0.4X\r\n", sReg1, pState->nCOP2VI[i] & 0xFFFF, sReg2, pState->nCOP2VI[i + 1] & 0xFFFF);
		(*pText) += sLine;
	}
}

void CRegViewVU::OnMachineStateChange(int nNothing)
{
	Update();
}

void CRegViewVU::OnRunningStateChange(int nNothing)
{
	Update();
}
