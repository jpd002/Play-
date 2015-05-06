#include <string.h>
#include "IPU_MotionCodeTable.h"

using namespace IPU;
using namespace MPEG2;

VLCTABLEENTRY CMotionCodeTable::m_pTable[ENTRYCOUNT] =
{
	{ 0x0001,		1,			0x00010000	},
	{ 0x0002,		3,			0x00030001	},
	{ 0x0003,		3,			0x0003FFFF	},
	{ 0x0002,		4,			0x00040002	},
	{ 0x0003,		4,			0x0004FFFE	},
	{ 0x0002,		5,			0x00050003	},
	{ 0x0003,		5,			0x0005FFFD	},
	{ 0x0006,		7,			0x00070004	},
	{ 0x0007,		7,			0x0007FFFC	},
	{ 0x000A,		8,			0x00080005	},
	{ 0x000B,		8,			0x0008FFFB	},
	{ 0x0008,		8,			0x00080006	},
	{ 0x0009,		8,			0x0008FFFA	},
	{ 0x0006,		8,			0x00080007	},
	{ 0x0007,		8,			0x0008FFF9	},
	{ 0x0016,		10,			0x000A0008	},
	{ 0x0017,		10,			0x000AFFF8	},
	{ 0x0014,		10,			0x000A0009	},
	{ 0x0015,		10,			0x000AFFF7	},
	{ 0x0012,		10,			0x000A000A	},
	{ 0x0013,		10,			0x000AFFF6	},
	{ 0x0022,		11,			0x000B000B	},
	{ 0x0023,		11,			0x000BFFF5	},
	{ 0x0020,		11,			0x000B000C	},
	{ 0x0021,		11,			0x000BFFF4	},
	{ 0x001E,		11,			0x000B000D	},
	{ 0x001F,		11,			0x000BFFF3	},
	{ 0x001C,		11,			0x000B000E	},
	{ 0x001D,		11,			0x000BFFF2	},
	{ 0x001A,		11,			0x000B000F	},
	{ 0x001B,		11,			0x000BFFF1	},
	{ 0x0018,		11,			0x000B0010	},
	{ 0x0019,		11,			0x000BFFF0	},
};

unsigned int CMotionCodeTable::m_pIndexTable[MAXBITS] =
{
	0,
	1,
	1,
	3,
	5,
	7,
	7,
	9,
	15,
	15,
	21,
};

CVLCTable* CMotionCodeTable::m_pInstance = NULL;

CMotionCodeTable::CMotionCodeTable() :
CVLCTable(MAXBITS, m_pTable, ENTRYCOUNT, m_pIndexTable)
{

}

CVLCTable* CMotionCodeTable::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new CMotionCodeTable();
	}

	return m_pInstance;
}
