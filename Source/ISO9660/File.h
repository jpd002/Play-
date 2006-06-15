#ifndef _ISO9660_FILE_H_
#define _ISO9660_FILE_H_

#include "Stream.h"
#include "ISO9660.h"

namespace ISO9660
{

	class CFile : public Framework::CStream
	{
	public:

							CFile(CISO9660*, uint64, uint64);
							~CFile();
		void				Seek(int64, Framework::STREAM_SEEK_DIRECTION);
		uint64				Tell();
		uint64				Read(void*, uint64);
		uint64				Write(const void*, uint64);
		bool				IsEOF();

	private:
		void				SyncBlock();

		CISO9660*			m_pISO;
		uint64				m_nStart;
		uint64				m_nEnd;
		uint64				m_nPosition;
		uint32				m_nBlockPosition;
		uint8				m_nBlock[CISO9660::BLOCKSIZE];
	};

}

#endif
