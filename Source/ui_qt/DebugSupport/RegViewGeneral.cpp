#include <stdio.h>
#include <string.h>
#include <QHeaderView>
#include "RegViewGeneral.h"

CRegViewGeneral::CRegViewGeneral(QWidget* hParent, CMIPS* pC)
    : CRegViewPage(hParent)
    , m_pCtx(pC)
{
	AllocateTableEntries(2, 37);
	setColumnWidth(0, 32);
	horizontalHeader()->setStretchLastSection(true);
	for(unsigned int x = 0; x < 37; x++)
	{
		setRowHeight(x, 16);
	}
	for(unsigned int x = 0; x < 32; x++)
	{
		WriteTableLabel(x, CMIPS::m_sGPRName[x]);
	}
	WriteTableLabel(32, "LO");
	WriteTableLabel(33, "HI");
	WriteTableLabel(34, "LO1");
	WriteTableLabel(35, "HI1");
	WriteTableLabel(36, "SA");

	Update();
}

void CRegViewGeneral::Update()
{
	MIPSSTATE* s = &m_pCtx->m_State;

	for(unsigned int i = 0; i < 32; i++)
	{
		WriteTableEntry(i, "0x%08X%08X%08X%08X", s->nGPR[i].nV[3], s->nGPR[i].nV[2], s->nGPR[i].nV[1], s->nGPR[i].nV[0]);
	}
	WriteTableEntry(32, "0x%08X%08X", s->nLO[1], s->nLO[0]);
	WriteTableEntry(33, "0x%08X%08X", s->nHI[1], s->nHI[0]);
	WriteTableEntry(34, "0x%08X%08X", s->nLO1[1], s->nLO1[0]);
	WriteTableEntry(35, "0x%08X%08X", s->nHI1[1], s->nHI1[0]);
	WriteTableEntry(36, "0x%08X", s->nSA);
}
