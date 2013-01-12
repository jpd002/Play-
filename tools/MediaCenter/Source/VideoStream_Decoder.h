#ifndef _VIDEOSTREAM_DECODER_H_
#define _VIDEOSTREAM_DECODER_H_

#include "VideoStream_Program.h"
#include "VideoStream_ReadPictureHeader.h"
#include "VideoStream_ReadPictureCodingExtension.h"
#include "VideoStream_ReadSequenceHeader.h"
#include "VideoStream_ReadSequenceExtension.h"
#include "VideoStream_ReadGroupOfPicturesHeader.h"
#include "VideoStream_ReadSlice.h"
#include "MpegVideoState.h"

namespace VideoStream
{
	class Decoder : public Program
	{
	public:		
		typedef ReadSlice::OnMacroblockDecodedHandler OnMacroblockDecodedHandler;
		
										Decoder();
		virtual							~Decoder();

		void							InitializeState(MPEG_VIDEO_STATE*);

		void							Reset();
		void							Execute(void*, Framework::CBitStream&);

		void							RegisterOnMacroblockDecodedHandler(const OnMacroblockDecodedHandler&);
		
	private:
		enum PROGRAM_STATE
		{
			STATE_GETMARKER = 0,
			STATE_SKIPMARKER,
			STATE_GETCOMMAND,
			STATE_GETEXTENSIONID,
			STATE_SKIPBITS,
			STATE_EXECUTESUBPROGRAM,
			STATE_FINISHEXECUTE,
		};

		PROGRAM_STATE					m_programState;
		uint32							m_marker;
		uint8							m_commandType;
		uint8							m_extensionType;

		Program*						m_subProgram;

		ReadPictureHeader				m_readPictureHeader;
		ReadPictureCodingExtension		m_readPictureCodingExtensionProgram;
		ReadSequenceExtension			m_readSequenceExtensionProgram;
		ReadSequenceHeader				m_readSequenceHeaderProgram;
		ReadGroupOfPicturesHeader		m_readGroupOfPicturesHeaderProgram;
		ReadSlice						m_readSliceProgram;
	};
}

#endif
