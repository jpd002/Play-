#ifndef _IPU_MACROBLOCKTYPEBTABLE_H_
#define _IPU_MACROBLOCKTYPEBTABLE_H_

#include "mpeg2/VLCTable.h"

namespace IPU
{
	class CMacroblockTypeBTable : public MPEG2::CVLCTable
	{
	public:
										CMacroblockTypeBTable();
		static CVLCTable*				GetInstance();

		enum MAXBITS
		{
			MAXBITS = 6,
		};

		enum ENTRYCOUNT
		{
			ENTRYCOUNT = 11,
		};

	private:
		static MPEG2::VLCTABLEENTRY		m_pTable[ENTRYCOUNT];
		static unsigned int				m_pIndexTable[MAXBITS];
		static MPEG2::CVLCTable*		m_pInstance;
	};
}

#endif
