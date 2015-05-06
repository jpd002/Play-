#include <stdio.h>
#include "RegViewVU.h"
#include "../PS2VM.h"

CRegViewVU::CRegViewVU(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* pCtx)
: CRegViewPage(hParent, rect)
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

	sprintf(sLine, "STKF : 0x%0.4X\r\n", pState->nCOP2SF);
	result += sLine;

	sprintf(sLine, "CLIP : 0x%0.6X\r\n", pState->nCOP2CF);
	result += sLine;

	sprintf(sLine, "PIPE : 0x%0.4X\r\n", pState->pipeTime);
	result += sLine;

	sprintf(sLine, "PIPEQ: 0x%0.4X - %+.7e\r\n", pState->pipeQ.counter, *(float*)&pState->pipeQ.heldValue);
	result += sLine;

	unsigned int currentPipeMacCounter = pState->pipeMac.index - 1;

	uint32 macFlagPipeValues[MACFLAG_PIPELINE_SLOTS];
	uint32 macFlagPipeTimes[MACFLAG_PIPELINE_SLOTS];
	for(unsigned int i = 0; i < MACFLAG_PIPELINE_SLOTS; i++)
	{
		unsigned int currIndex = (currentPipeMacCounter - i) & (MACFLAG_PIPELINE_SLOTS - 1);
		macFlagPipeValues[i] = pState->pipeMac.values[currIndex];
		macFlagPipeTimes[i] = pState->pipeMac.pipeTimes[currIndex];
	}

	sprintf(sLine, "PIPEM: 0x%0.4X:0x%0.4X, 0x%0.4X:0x%0.4X\r\n", 
		macFlagPipeTimes[0], macFlagPipeValues[0],
		macFlagPipeTimes[1], macFlagPipeValues[1]);
	result += sLine;

	sprintf(sLine, "       0x%0.4X:0x%0.4X, 0x%0.4X:0x%0.4X\r\n", 
		macFlagPipeTimes[2], macFlagPipeValues[2],
		macFlagPipeTimes[3], macFlagPipeValues[3]);
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
