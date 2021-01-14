#include <stdio.h>
#include <string.h>
#include <QHeaderView>
#include <QMenu>
#include "RegViewFPU.h"

CRegViewFPU::CRegViewFPU(QWidget* parent, CMIPS* pC)
    : CRegViewPage(parent)
    , m_pCtx(pC)
    , m_nViewMode(VIEWMODE_SINGLE)
{
	// Setup the register names
	AllocateTableEntries(2, 35);
	setColumnWidth(0, 40);
	horizontalHeader()->setStretchLastSection(true);
	for(unsigned int x = 0; x < 32; x++)
	{
		setRowHeight(x, 16);
		WriteTableLabel(x, "F%d", x);
	}
	WriteTableLabel(32, "ACC");
	WriteTableLabel(33, "FCSR");
	WriteTableLabel(34, "CC");
	Update();

	// Add a context menu for switching between modes
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &CRegViewFPU::customContextMenuRequested, this, &CRegViewFPU::ShowContextMenu);
}

void CRegViewFPU::Update()
{
	switch(m_nViewMode)
	{
	case VIEWMODE_WORD:
		RenderWord();
		break;
	case VIEWMODE_SINGLE:
		RenderSingle();
		break;
	default:
		break;
	}
	RenderFCSR();
}

void CRegViewFPU::RenderFCSR()
{
	WriteTableEntry(33, "0x%08X", m_pCtx->m_State.nFCSR);
	WriteTableEntry(34, "%i%i%i%i%i%i%i%ib",
	                (m_pCtx->m_State.nFCSR & 0x80000000) != 0 ? 1 : 0,
	                (m_pCtx->m_State.nFCSR & 0x40000000) != 0 ? 1 : 0,
	                (m_pCtx->m_State.nFCSR & 0x20000000) != 0 ? 1 : 0,
	                (m_pCtx->m_State.nFCSR & 0x10000000) != 0 ? 1 : 0,
	                (m_pCtx->m_State.nFCSR & 0x04000000) != 0 ? 1 : 0,
	                (m_pCtx->m_State.nFCSR & 0x08000000) != 0 ? 1 : 0,
	                (m_pCtx->m_State.nFCSR & 0x02000000) != 0 ? 1 : 0,
	                (m_pCtx->m_State.nFCSR & 0x00800000) != 0 ? 1 : 0);
}

void CRegViewFPU::RenderWord()
{
	MIPSSTATE* s = &m_pCtx->m_State;

	for(unsigned int i = 0; i < 32; i++)
	{
		WriteTableEntry(i, "0x%08X", ((uint32*)s->nCOP1)[i]);
	}
}

void CRegViewFPU::RenderSingle()
{
	MIPSSTATE* s = &m_pCtx->m_State;

	for(unsigned int i = 0; i < 32; i++)
	{
		uint32 nData = ((uint32*)s->nCOP1)[i];
		float nValue = *(float*)(&nData);

		WriteTableEntry(i, "%+.24e", nValue);
	}
	WriteTableEntry(32, "%+.24e", *(float*)(&s->nCOP1A));
}

void CRegViewFPU::ShowContextMenu(const QPoint& pos)
{
	QMenu contextMenu("Context menu", this);
	contextMenu.addAction("Word Mode", [&]() {m_nViewMode=VIEWMODE_WORD; Update(); });
	contextMenu.addAction("Single Mode", [&]() {m_nViewMode=VIEWMODE_SINGLE; Update(); });
	contextMenu.exec(mapToGlobal(pos));
}

void CRegViewFPU::OnRunningStateChange()
{
	Update();
}

void CRegViewFPU::OnMachineStateChange()
{
	Update();
}
