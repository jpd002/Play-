#ifndef _VIDEOSTREAM_READDCT_H_
#define _VIDEOSTREAM_READDCT_H_

#include "MPEG2/DctCoefficientTable.h"
#include "VideoStream_ReadDcDifferential.h"

namespace VideoStream
{
	class ReadDct
	{
	public:
		ReadDct();
		virtual ~ReadDct();

		void Reset();
		void Execute(void*, Framework::CBitStream&);
		void SetBlockInfo(int16*, unsigned int);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READDCDIFFERENTIAL,
			STATE_CHECKEOB,
			STATE_READCOEFF,
			STATE_SKIPEOB,
			STATE_DONE,
		};

		PROGRAM_STATE m_programState;
		MPEG2::CDctCoefficientTable* m_coeffTable;
		ReadDcDifferential m_dcDiffReader;
		unsigned int m_blockIndex;
		int16* m_block;
		unsigned int m_channel;
	};
}

#endif
