#include <string.h>
#include "IPU_MacroblockTypePTable.h"

using namespace IPU;
using namespace MPEG2;

VLCTABLEENTRY CMacroblockTypePTable::m_pTable[ENTRYCOUNT] =
{
	{ 0x0001,		1,			0x0001000A	},
	{ 0x0001,		2,			0x00020002	},
	{ 0x0001,		3,			0x00030008	},
	{ 0x0003,		5,			0x00050001	},
	{ 0x0002,		5,			0x0005001A	},
	{ 0x0001,		5,			0x00050012	},
	{ 0x0001,		6,			0x00060011	},
};

unsigned int CMacroblockTypePTable::m_pIndexTable[MAXBITS] =
{
	0,
	1,
	2,
	2,
	3,
	6,
};

CVLCTable* CMacroblockTypePTable::m_pInstance = NULL;

CMacroblockTypePTable::CMacroblockTypePTable() :
CVLCTable(MAXBITS, m_pTable, ENTRYCOUNT, m_pIndexTable)
{

}

CVLCTable* CMacroblockTypePTable::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new CMacroblockTypePTable();
	}

	return m_pInstance;
}
