#include <stdio.h>
#include <string.h>
#include <QHeaderView>
#include "RegViewSCU.h"
#include "COP_SCU.h"

CRegViewSCU::CRegViewSCU(QWidget* parent, CMIPS* ctx)
    : CRegViewPage(parent)
    , m_ctx(ctx)
{
	this->AllocateTableEntries(2, 32);
	this->setColumnWidth(0, 85);
	this->horizontalHeader()->setStretchLastSection(true);
	for(unsigned int x = 0; x < 32; x++)
	{
		this->setRowHeight(x, 16);
		this->WriteTableLabel(x, CCOP_SCU::m_sRegName[x]);
	}
	this->Update();
}

void CRegViewSCU::Update()
{
	const auto& state = m_ctx->m_State;

	for(unsigned int i = 0; i < 32; i++)
	{
		this->WriteTableEntry(i, "0x%08X", state.nCOP0[i]);
	}

}
