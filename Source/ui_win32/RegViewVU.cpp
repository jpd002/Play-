#include <stdio.h>
#include "RegViewVU.h"
#include "string_format.h"
#include "../PS2VM.h"

#define MENUCMD_BASE 40000
#define CLIP_FLAG_MASK 0xFFFFFF

CRegViewVU::CRegViewVU(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* ctx)
: CRegViewPage(hParent, rect)
, m_ctx(ctx)
{
	virtualMachine.OnMachineStateChange.connect(boost::bind(&CRegViewVU::Update, this));
	virtualMachine.OnRunningStateChange.connect(boost::bind(&CRegViewVU::Update, this));

	Update();
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

	const auto& state = m_ctx->m_State;

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

		if(m_viewMode == VIEWMODE_WORD)
		{
			sprintf(sLine, "%s:    0x%0.8X      0x%0.8X\r\n          0x%0.8X      0x%0.8X\r\n", sReg1,
				state.nCOP2[i].nV0, state.nCOP2[i].nV1,
				state.nCOP2[i].nV2, state.nCOP2[i].nV3
			);
		}
		else if(m_viewMode == VIEWMODE_SINGLE)
		{
			sprintf(sLine, "%s: %+.7e %+.7e\r\n       %+.7e %+.7e\r\n", sReg1,
				*reinterpret_cast<const float*>(&state.nCOP2[i].nV0),
				*reinterpret_cast<const float*>(&state.nCOP2[i].nV1),
				*reinterpret_cast<const float*>(&state.nCOP2[i].nV2),
				*reinterpret_cast<const float*>(&state.nCOP2[i].nV3)
			);
		}
		else
		{
			assert(false);
		}

		result += sLine;
	}

	if(m_viewMode == VIEWMODE_WORD)
	{
		sprintf(sLine, "ACC  :    0x%0.8X      0x%0.8X\r\n          0x%0.8X      0x%0.8X\r\n",
			state.nCOP2A.nV0, state.nCOP2A.nV1,
			state.nCOP2A.nV2, state.nCOP2A.nV3
		);
	}
	else if(m_viewMode == VIEWMODE_SINGLE)
	{
		sprintf(sLine, "ACC  : %+.7e %+.7e\r\n       %+.7e %+.7e\r\n",
			*reinterpret_cast<const float*>(&state.nCOP2A.nV0),
			*reinterpret_cast<const float*>(&state.nCOP2A.nV1),
			*reinterpret_cast<const float*>(&state.nCOP2A.nV2),
			*reinterpret_cast<const float*>(&state.nCOP2A.nV3)
		);
	}
	else
	{
		assert(false);
	}

	result += sLine;

	if(m_viewMode == VIEWMODE_WORD)
	{
		sprintf(sLine, "Q    : 0x%0.8X\r\n", state.nCOP2Q);
		result += sLine;

		sprintf(sLine, "I    : 0x%0.8X\r\n", state.nCOP2I);
		result += sLine;

		sprintf(sLine, "P    : 0x%0.8X\r\n", state.nCOP2P);
		result += sLine;
	}
	else if(m_viewMode == VIEWMODE_SINGLE)
	{
		sprintf(sLine, "Q    : %+.7e\r\n", *reinterpret_cast<const float*>(&state.nCOP2Q));
		result += sLine;

		sprintf(sLine, "I    : %+.7e\r\n", *reinterpret_cast<const float*>(&state.nCOP2I));
		result += sLine;

		sprintf(sLine, "P    : %+.7e\r\n", *reinterpret_cast<const float*>(&state.nCOP2P));
		result += sLine;
	}
	else
	{
		assert(false);
	}

	sprintf(sLine, "R    : %+.7e (0x%0.8X)\r\n", *reinterpret_cast<const float*>(&state.nCOP2R), state.nCOP2R);
	result += sLine;

	sprintf(sLine, "MACF : 0x%0.4X\r\n", state.nCOP2MF);
	result += sLine;

	sprintf(sLine, "STKF : 0x%0.4X\r\n", state.nCOP2SF);
	result += sLine;

	sprintf(sLine, "CLIP : 0x%0.6X\r\n", state.nCOP2CF & CLIP_FLAG_MASK);
	result += sLine;

	sprintf(sLine, "PIPE : 0x%0.4X\r\n", state.pipeTime);
	result += sLine;

	sprintf(sLine, "PIPEQ: 0x%0.4X - %+.7e\r\n", state.pipeQ.counter, *reinterpret_cast<const float*>(&state.pipeQ.heldValue));
	result += sLine;

	result += PrintPipeline("PIPEM:", state.pipeMac);

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

		sprintf(sLine, "%s: 0x%0.4X    %s: 0x%0.4X\r\n", sReg1, state.nCOP2VI[i] & 0xFFFF, sReg2, state.nCOP2VI[i + 1] & 0xFFFF);
		result += sLine;
	}

	return result;
}

std::string CRegViewVU::PrintPipeline(const char* title, const MACFLAG_PIPELINE& pipe)
{
	//Print pipeline in reverse order
	//Only the first 24-bits of values are printed because
	//this is used for the clip register

	std::string result;
	unsigned int currentPipeMacCounter = pipe.index - 1;

	uint32 pipeValues[MACFLAG_PIPELINE_SLOTS];
	uint32 pipeTimes[MACFLAG_PIPELINE_SLOTS];
	for(unsigned int i = 0; i < MACFLAG_PIPELINE_SLOTS; i++)
	{
		unsigned int currIndex = (currentPipeMacCounter - i) & (MACFLAG_PIPELINE_SLOTS - 1);
		pipeValues[i] = pipe.values[currIndex] & CLIP_FLAG_MASK;
		pipeTimes[i] = pipe.pipeTimes[currIndex];
	}

	for(unsigned int i = 0; i < (MACFLAG_PIPELINE_SLOTS / 2); i++)
	{
		const char* front = (i == 0) ? title : "      ";
		result += string_format("%s 0x%0.4X:0x%0.6X, 0x%0.4X:0x%0.6X\r\n", front,
			pipeTimes[(i * 2) + 0], pipeValues[(i * 2) + 0],
			pipeTimes[(i * 2) + 1], pipeValues[(i * 2) + 1]);
	}

	return result;
}

long CRegViewVU::OnRightButtonUp(int nX, int nY)
{
	POINT pt = { nX, nY };
	ClientToScreen(m_hWnd, &pt);

	HMENU hMenu = CreatePopupMenu();
	InsertMenu(hMenu, 0, MF_BYPOSITION | (m_viewMode == VIEWMODE_WORD ? MF_CHECKED : 0),   MENUCMD_BASE + VIEWMODE_WORD,   _T("32 Bits Integers"));
	InsertMenu(hMenu, 1, MF_BYPOSITION | (m_viewMode == VIEWMODE_SINGLE ? MF_CHECKED : 0), MENUCMD_BASE + VIEWMODE_SINGLE, _T("Single Precision Floating-Point Numbers"));

	TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_hWnd, NULL); 

	return FALSE;
}

long CRegViewVU::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	if((nID >= MENUCMD_BASE) && (nID < (MENUCMD_BASE + VIEWMODE_MAX)))
	{
		m_viewMode = static_cast<VIEWMODE>(nID - MENUCMD_BASE);
		Update();
	}

	return TRUE;
}
