#ifndef _VIDEOSTREAM_READSEQUENCEHEADER_H_
#define _VIDEOSTREAM_READSEQUENCEHEADER_H_

#include "MpegVideoState.h"
#include "VideoStream_Program.h"
#include "VideoStream_ReadStructure.h"

namespace VideoStream
{
	class ReadSequenceHeader : public Program
	{
	public:
		ReadSequenceHeader();
		virtual ~ReadSequenceHeader();

		void Reset();
		void Execute(void*, Framework::CBitStream&);

		class ReadSequenceHeaderStruct : public ReadStructure<SEQUENCE_HEADER>
		{
		public:
			ReadSequenceHeaderStruct();
			virtual ~ReadSequenceHeaderStruct();
		};

		class QuantizerMatrixReader : public Program
		{
		public:
			QuantizerMatrixReader();
			virtual ~QuantizerMatrixReader();

			void Reset();
			void Execute(void*, Framework::CBitStream&);
			void SetTable(uint8*);

		private:
			uint8* m_table;
			unsigned int m_currentIndex;
		};

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READSTRUCT,
			STATE_CHECKREADINTRAMATRIX,
			STATE_READINTRAMATRIX,
			STATE_CHECKREADNONINTRAMATRIX,
			STATE_READNONINTRAMATRIX,
			STATE_DONE,
		};

		PROGRAM_STATE m_programState;
		ReadSequenceHeaderStruct m_structureReader;
		QuantizerMatrixReader m_quantizerMatrixReader;
	};
}

#endif
