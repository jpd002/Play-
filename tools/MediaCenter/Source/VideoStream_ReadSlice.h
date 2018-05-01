#ifndef _VIDEOSTREAM_READSLICE_H_
#define _VIDEOSTREAM_READSLICE_H_

#include <functional>
#include "VideoStream_Program.h"
#include "VideoStream_ReadMacroblock.h"

namespace VideoStream
{
	class ReadSlice : public Program
	{
	public:
		typedef ReadMacroblock::OnMacroblockDecodedHandler OnMacroblockDecodedHandler;
		typedef std::function<void(MPEG_VIDEO_STATE*)> OnPictureDecodedHandler;

		ReadSlice();
		virtual ~ReadSlice();

		void Reset();
		void Execute(void*, Framework::CBitStream&);

		void RegisterOnMacroblockDecodedHandler(const OnMacroblockDecodedHandler&);
		void RegisterOnPictureDecodedHandler(const OnPictureDecodedHandler&);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READQUANTIZERSCALECODE,
			STATE_READEXTRASLICEINFOFLAG,
			STATE_CHECKEND,
			STATE_READMACROBLOCK,
			STATE_DONE,
		};

		PROGRAM_STATE m_programState;
		ReadMacroblock m_macroblockReader;
		OnPictureDecodedHandler m_OnPictureDecodedHandler;
	};
}

#endif
