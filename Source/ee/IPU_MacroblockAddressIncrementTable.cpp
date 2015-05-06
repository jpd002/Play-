#include <string.h>
#include "IPU_MacroblockAddressIncrementTable.h"

using namespace IPU;
using namespace MPEG2;

VLCTABLEENTRY CMacroblockAddressIncrementTable::m_pTable[ENTRYCOUNT] =
{
	{ 0x0001,		1,			0x00010001	},
	{ 0x0003,		3,			0x00030002	},
	{ 0x0002,		3,			0x00030003	},
	{ 0x0003,		4,			0x00040004	},
	{ 0x0002,		4,			0x00040005	},
	{ 0x0003,		5,			0x00050006	},
	{ 0x0002,		5,			0x00050007	},
	{ 0x0007,		7,			0x00070008	},
	{ 0x0006,		7,			0x00070009	},
	{ 0x000B,		8,			0x0008000A	},
	{ 0x000A,		8,			0x0008000B	},
	{ 0x0009,		8,			0x0008000C	},
	{ 0x0008,		8,			0x0008000D	},
	{ 0x0007,		8,			0x0008000E	},
	{ 0x0006,		8,			0x0008000F	},
	{ 0x0017,		10,			0x000A0010	},
	{ 0x0016,		10,			0x000A0011	},
	{ 0x0015,		10,			0x000A0012	},
	{ 0x0014,		10,			0x000A0013	},
	{ 0x0013,		10,			0x000A0014	},
	{ 0x0012,		10,			0x000A0015	},
	{ 0x0023,		11,			0x000B0016	},
	{ 0x0022,		11,			0x000B0017	},
	{ 0x0021,		11,			0x000B0018	},
	{ 0x0020,		11,			0x000B0019	},
	{ 0x001F,		11,			0x000B001A	},
	{ 0x001E,		11,			0x000B001B	},
	{ 0x001D,		11,			0x000B001C	},
	{ 0x001C,		11,			0x000B001D	},
	{ 0x001B,		11,			0x000B001E	},
	{ 0x001A,		11,			0x000B001F	},
	{ 0x0019,		11,			0x000B0020	},
	{ 0x0018,		11,			0x000B0021	},
	{ 0x000F,		11,			0x000B0022	},
	{ 0x0008,		11,			0x000B0023	},
};

unsigned int CMacroblockAddressIncrementTable::m_pIndexTable[MAXBITS] =
{
	0,
	1,
	1,
	3,
	5,
	5,
	7,
	9,
	9,
	15,
	21,
};

CVLCTable* CMacroblockAddressIncrementTable::m_pInstance = NULL;

CMacroblockAddressIncrementTable::CMacroblockAddressIncrementTable() :
CVLCTable(MAXBITS, m_pTable, ENTRYCOUNT, m_pIndexTable)
{

}

CVLCTable* CMacroblockAddressIncrementTable::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new CMacroblockAddressIncrementTable();
	}

	return m_pInstance;
}
