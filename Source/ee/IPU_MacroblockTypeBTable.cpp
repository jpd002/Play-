#include <string.h>
#include "IPU_MacroblockTypeBTable.h"

using namespace IPU;
using namespace MPEG2;

VLCTABLEENTRY CMacroblockTypeBTable::m_pTable[ENTRYCOUNT] =
{
	{ 0x0002,		2,			0x0002000C	},
	{ 0x0003,		2,			0x0002000E	},
	{ 0x0002,		3,			0x00030004	},
	{ 0x0003,		3,			0x00030006	},
	{ 0x0002,		4,			0x00040008	},
	{ 0x0003,		4,			0x0004000A	},
	{ 0x0003,		5,			0x00050001	},
	{ 0x0002,		5,			0x0005001E	},
	{ 0x0003,		6,			0x0006001A	},
	{ 0x0002,		6,			0x00060016	},
	{ 0x0001,		6,			0x00060011	},
};

unsigned int CMacroblockTypeBTable::m_pIndexTable[MAXBITS] =
{
	0,
	0,
	2,
	4,
	6,
	8,
};

CVLCTable* CMacroblockTypeBTable::m_pInstance = NULL;

CMacroblockTypeBTable::CMacroblockTypeBTable() :
CVLCTable(MAXBITS, m_pTable, ENTRYCOUNT, m_pIndexTable)
{

}

CVLCTable* CMacroblockTypeBTable::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new CMacroblockTypeBTable();
	}

	return m_pInstance;
}
