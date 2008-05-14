#include <stdio.h>
#include <string.h>
#include <boost/bind.hpp>
#include "RegViewSCU.h"
#include "../COP_SCU.h"

using namespace Framework;
using namespace boost;
using namespace std;

CRegViewSCU::CRegViewSCU(HWND hParent, RECT* pR, CVirtualMachine& virtualMachine, CMIPS* pC) :
CRegViewPage(hParent, pR)
{
	m_pCtx = pC;

	virtualMachine.m_OnMachineStateChange.connect(bind(&CRegViewSCU::Update, this));
	virtualMachine.m_OnRunningStateChange.connect(bind(&CRegViewSCU::Update, this));
}

CRegViewSCU::~CRegViewSCU()
{

}

void CRegViewSCU::Update()
{
	SetDisplayText(GetDisplayText().c_str());
	CRegViewPage::Update();
}

string CRegViewSCU::GetDisplayText()
{
	MIPSSTATE* s = &m_pCtx->m_State;
    string result;

	for(unsigned int i = 0; i < 32; i++)
	{
	    char sTemp[256];
		sprintf(sTemp, "%10s : 0x%0.8X\r\n", CCOP_SCU::m_sRegName[i], s->nCOP0[i]);
		result += sTemp;
	}

    return result;
}
