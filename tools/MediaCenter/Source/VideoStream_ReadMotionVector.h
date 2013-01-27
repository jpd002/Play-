#ifndef _VIDEOSTREAM_READMOTIONVECTOR_H_
#define _VIDEOSTREAM_READMOTIONVECTOR_H_

#include "VideoStream_Program.h"

namespace VideoStream
{
	class ReadMotionVector : public Program
	{
	public:
							ReadMotionVector();
		virtual				~ReadMotionVector();

		void				SetRSizes(uint8, uint8);

		void				Reset();
		void				Execute(void*, Framework::CBitStream&);

		static int16		ComputeMotionVector(int16, int16, uint16, uint8);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READHMOTIONCODE,
			STATE_READHMOTIONRESIDUAL,
			STATE_READVMOTIONCODE,
			STATE_READVMOTIONRESIDUAL,
			STATE_DONE,
		};

		PROGRAM_STATE		m_programState;
		uint8				m_hrSize;
		uint8				m_vrSize;
	};
}

#endif
