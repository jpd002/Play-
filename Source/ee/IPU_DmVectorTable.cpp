#include <string.h>
#include "IPU_DmVectorTable.h"

using namespace IPU;
using namespace MPEG2;

VLCTABLEENTRY CDmVectorTable::m_pTable[ENTRYCOUNT] =
{
	{ 0x0000,		1,			0x00010000	},
	{ 0x0002,		2,			0x00020001	},
	{ 0x0003,		2,			0x0002FFFF	},
};

unsigned int CDmVectorTable::m_pIndexTable[MAXBITS] =
{
	0,
	1,
};

CVLCTable* CDmVectorTable::m_pInstance = NULL;

CDmVectorTable::CDmVectorTable() :
CVLCTable(MAXBITS, m_pTable, ENTRYCOUNT, m_pIndexTable)
{

}

CVLCTable* CDmVectorTable::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new CDmVectorTable();
	}

	return m_pInstance;
}
