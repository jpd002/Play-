#include <stdio.h>
#include <string.h>
#include "RegViewSCU.h"
#include "../COP_SCU.h"

CRegViewSCU::CRegViewSCU(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* pC) 
: CRegViewPage(hParent, rect)
, m_pCtx(pC)
{
	virtualMachine.OnMachineStateChange.connect(boost::bind(&CRegViewSCU::Update, this));
	virtualMachine.OnRunningStateChange.connect(boost::bind(&CRegViewSCU::Update, this));
}

CRegViewSCU::~CRegViewSCU()
{

}

void CRegViewSCU::Update()
{
	SetDisplayText(GetDisplayText().c_str());
	CRegViewPage::Update();
}

std::string CRegViewSCU::GetDisplayText()
{
	MIPSSTATE* s = &m_pCtx->m_State;
	std::string result;

	for(unsigned int i = 0; i < 32; i++)
	{
		char sTemp[256];
		sprintf(sTemp, "%10s : 0x%0.8X\r\n", CCOP_SCU::m_sRegName[i], s->nCOP0[i]);
		result += sTemp;
	}

	return result;
}
