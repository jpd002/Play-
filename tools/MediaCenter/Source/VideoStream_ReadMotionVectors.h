#ifndef _VIDEOSTREAM_READMOTIONVECTORS_H_
#define _VIDEOSTREAM_READMOTIONVECTORS_H_

#include "VideoStream_Program.h"
#include "VideoStream_ReadMotionVector.h"

namespace VideoStream
{
	class ReadMotionVectors : public Program
	{
	public:
							ReadMotionVectors();
		virtual				~ReadMotionVectors();

		void				SetRSizes(uint8, uint8);
		void				SetMotionVector(int16*);

		void				Reset();
		void				Execute(void*, Framework::CBitStream&);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_SINGLE_READVECTOR,
			STATE_DOUBLE_FIRST_READFIELDSELECT,
			STATE_DOUBLE_FIRST_READVECTOR,
			STATE_DOUBLE_SECOND_READFIELDSELECT,
			STATE_DOUBLE_SECOND_READVECTOR,
			STATE_DONE,
		};

		ReadMotionVector	m_motionVectorReader;
		PROGRAM_STATE		m_programState;
		uint8				m_hrSize;
		uint8				m_vrSize;
		int16*				m_motionVector;
	};
}

#endif
