#include <stdio.h>
#include "RegViewVU.h"
#include "../PS2VM.h"

CRegViewVU::CRegViewVU(HWND hParent, RECT* pR, CVirtualMachine& virtualMachine, CMIPS* pCtx)
: CRegViewPage(hParent, pR)
, m_pCtx(pCtx)
{
	virtualMachine.OnMachineStateChange.connect(boost::bind(&CRegViewVU::Update, this));
	virtualMachine.OnRunningStateChange.connect(boost::bind(&CRegViewVU::Update, this));

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

std::string CRegViewVU::GetDisplayText()
{
	char sLine[256];
	std::string result;

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

	sprintf(sLine, "MACF : 0x%0.4X\r\n", pState->nCOP2MF);
	result += sLine;

	sprintf(sLine, "CLIP : 0x%0.6X\r\n", pState->nCOP2CF);
	result += sLine;

	sprintf(sLine, "PIPEQ: 0x%0.4X - %+.7e\r\n", pState->pipeQ.counter, *(float*)&pState->pipeQ.heldValue);
	result += sLine;

	unsigned int currentPipeMacCounter = pState->pipeMac.counter;

	uint32 macFlagPipeValues[MACFLAG_PIPELINE_SLOTS];
	for(unsigned int i = 0; i < MACFLAG_PIPELINE_SLOTS; i++)
	{
		macFlagPipeValues[i] = pState->pipeMac.slots[(i + currentPipeMacCounter) & (MACFLAG_PIPELINE_SLOTS - 1)];
	}

	sprintf(sLine, "PIPEM:%s0x%0.4X,%s0x%0.4X,%s0x%0.4X,%s0x%0.4X\r\n", 
		(macFlagPipeValues[0] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[0] & 0xFFFF, 
		(macFlagPipeValues[1] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[1] & 0xFFFF, 
		(macFlagPipeValues[2] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[2] & 0xFFFF, 
		(macFlagPipeValues[3] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[3] & 0xFFFF);
	result += sLine;

	sprintf(sLine, "      %s0x%0.4X,%s0x%0.4X,%s0x%0.4X,%s0x%0.4X\r\n", 
		(macFlagPipeValues[4] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[4] & 0xFFFF, 
		(macFlagPipeValues[5] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[5] & 0xFFFF, 
		(macFlagPipeValues[6] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[6] & 0xFFFF, 
		(macFlagPipeValues[7] & 0x80000000) == 0 ? " " : "*", macFlagPipeValues[7] & 0xFFFF);
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
