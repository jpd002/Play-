#ifndef _IPU_DMVECTORTABLE_H_
#define _IPU_DMVECTORTABLE_H_

#include "mpeg2/VLCTable.h"

namespace IPU
{

	class CDmVectorTable : public MPEG2::CVLCTable
	{
	public:
										CDmVectorTable();
		static MPEG2::CVLCTable*		GetInstance();

		enum MAXBITS
		{
			MAXBITS = 2,
		};

		enum ENTRYCOUNT
		{
			ENTRYCOUNT = 3,
		};

	private:
		static MPEG2::VLCTABLEENTRY		m_pTable[ENTRYCOUNT];
		static unsigned int				m_pIndexTable[MAXBITS];
		static MPEG2::CVLCTable*		m_pInstance;
	};

}

#endif
