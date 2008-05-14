#include <stdio.h>
#include <boost/bind.hpp>
#include "RegViewVU.h"
#include "../PS2VM.h"

using namespace Framework;
using namespace boost;
using namespace std;

CRegViewVU::CRegViewVU(HWND hParent, RECT* pR, CVirtualMachine& virtualMachine, CMIPS* pCtx) :
CRegViewPage(hParent, pR)
{
	m_pCtx = pCtx;

	virtualMachine.m_OnMachineStateChange.connect(bind(&CRegViewVU::Update, this));
	virtualMachine.m_OnRunningStateChange.connect(bind(&CRegViewVU::Update, this));

	Update();
}

CRegViewVU::~CRegViewVU()
{

}

void CRegViewVU::Update()
{
	SetDisplayText(GetDisplayText().c_str());
	CRegViewPage::Update();
}

string CRegViewVU::GetDisplayText()
{
	char sLine[256];
    string result;

	result += "              x               y       \r\n";
	result += "              z               w       \r\n";

	MIPSSTATE* pState = &m_pCtx->m_State;

	for(unsigned int i = 0; i < 32; i++)
	{
	    char sReg1[32];

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

		result += sLine;
	}

	sprintf(sLine, "ACC  : %+.7e %+.7e\r\n       %+.7e %+.7e\r\n", \
		*(float*)&pState->nCOP2A.nV0, \
		*(float*)&pState->nCOP2A.nV1, \
		*(float*)&pState->nCOP2A.nV2, \
		*(float*)&pState->nCOP2A.nV3);

	result += sLine;

	sprintf(sLine, "Q    : %+.7e\r\n", *(float*)&pState->nCOP2Q);
	result += sLine;

	sprintf(sLine, "I    : %+.7e\r\n", *(float*)&pState->nCOP2I);
	result += sLine;

	sprintf(sLine, "P    : %+.7e\r\n", *(float*)&pState->nCOP2P);
	result += sLine;

	sprintf(sLine, "R    : %+.7e (0x%0.8X)\r\n", *(float*)&pState->nCOP2R, pState->nCOP2R);
	result += sLine;

	sprintf(sLine, "MACSF: %i%i%i%ib\r\n", \
		(pState->nCOP2SF.nV0 != 0) ? 1 : 0, \
		(pState->nCOP2SF.nV1 != 0) ? 1 : 0, \
		(pState->nCOP2SF.nV2 != 0) ? 1 : 0, \
		(pState->nCOP2SF.nV3 != 0) ? 1 : 0);
	result += sLine;

	sprintf(sLine, "MACZF: %i%i%i%ib\r\n", \
		(pState->nCOP2ZF.nV0 != 0) ? 1 : 0, \
		(pState->nCOP2ZF.nV1 != 0) ? 1 : 0, \
		(pState->nCOP2ZF.nV2 != 0) ? 1 : 0, \
		(pState->nCOP2ZF.nV3 != 0) ? 1 : 0);
	result += sLine;

	sprintf(sLine, "CLIP : 0x%0.6X\r\n", pState->nCOP2CF);
	result += sLine;

	for(unsigned int i = 0; i < 16; i += 2)
	{
	    char sReg1[32];
	    char sReg2[32];

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
		result += sLine;
	}

    return result;
}
