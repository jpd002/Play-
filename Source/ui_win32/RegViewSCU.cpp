#include <stdio.h>
#include <string.h>
#include "RegViewSCU.h"
#include "../COP_SCU.h"

CRegViewSCU::CRegViewSCU(HWND parentWnd, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* ctx)
    : CRegViewPage(parentWnd, rect)
    , m_ctx(ctx)
{
	virtualMachine.OnMachineStateChange.connect(boost::bind(&CRegViewSCU::Update, this));
	virtualMachine.OnRunningStateChange.connect(boost::bind(&CRegViewSCU::Update, this));
}

void CRegViewSCU::Update()
{
	SetDisplayText(GetDisplayText().c_str());
	CRegViewPage::Update();
}

std::string CRegViewSCU::GetDisplayText()
{
	const auto& state = m_ctx->m_State;
	std::string result;

	for(unsigned int i = 0; i < 32; i++)
	{
		char sTemp[256];
		sprintf(sTemp, "%10s : 0x%08X\r\n", CCOP_SCU::m_sRegName[i], state.nCOP0[i]);
		result += sTemp;
	}

	return result;
}
