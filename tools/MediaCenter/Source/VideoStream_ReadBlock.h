#ifndef _VIDEOSTREAM_READBLOCK_H_
#define _VIDEOSTREAM_READBLOCK_H_

#include "VideoStream_Program.h"
#include "VideoStream_ReadDct.h"

namespace VideoStream
{
	class ReadBlock : public Program
	{
	public:
						ReadBlock();
		virtual			~ReadBlock();

		void			Reset();
		void			Execute(void*, Framework::CBitStream&);

	private:
		struct BLOCKENTRY
		{
			int16*			block;
			unsigned int	channel;
		};

		enum PROGRAM_STATE
		{
			STATE_RESETDC,
			STATE_INITDECODE,
			STATE_DECODE,
			STATE_MOVENEXT,
			STATE_DONE,
		};
		
		void			ProcessBlock(void*, int16*);
		void			DequantizeBlock(void*, int16*);
		void			InverseScan(void*, int16*);

		PROGRAM_STATE	m_programState;
		unsigned int	m_currentBlockIndex;
		ReadDct			m_dctReader;
	};
}

#endif
