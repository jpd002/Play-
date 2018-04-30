#ifndef _VIDEOSTREAM_READMACROBLOCK_H_
#define _VIDEOSTREAM_READMACROBLOCK_H_

#include <functional>
#include "MpegVideoState.h"
#include "VideoStream_Program.h"
#include "VideoStream_ReadMacroblockModes.h"
#include "VideoStream_ReadMotionVectors.h"
#include "VideoStream_ReadBlock.h"

namespace VideoStream
{
	class ReadMacroblock
	{
	public:
		typedef std::function<void(MPEG_VIDEO_STATE*)> OnMacroblockDecodedHandler;

		ReadMacroblock();
		virtual ~ReadMacroblock();

		void Reset();
		void Execute(void*, Framework::CBitStream&);

		void RegisterOnMacroblockDecodedHandler(const OnMacroblockDecodedHandler&);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_ESCAPE,
			STATE_SKIPESCAPE,
			STATE_READMBINCREMENT,
			STATE_READMBMODES,
			STATE_READDCTTYPE,
			STATE_CHECKMBMODES,
			STATE_CHECKMBMODES_QSC,
			STATE_CHECKMBMODES_FWM_INIT,
			STATE_CHECKMBMODES_FWM,
			STATE_CHECKMBMODES_BKM_INIT,
			STATE_CHECKMBMODES_BKM,
			STATE_CHECKMBMODES_CBP,
			STATE_READBLOCKINIT,
			STATE_READBLOCK,
		};

		ReadMacroblockModes m_macroblockModesReader;
		ReadMotionVectors m_motionVectorsReader;
		ReadMotionVector m_singleMotionVectorReader;
		ReadBlock m_blockReader;

		PROGRAM_STATE m_programState;

		OnMacroblockDecodedHandler m_OnMacroblockDecodedHandler;
	};
};

#endif
