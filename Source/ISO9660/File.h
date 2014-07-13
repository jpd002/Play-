#pragma once

#include "Stream.h"
#include "ISO9660.h"

namespace ISO9660
{

	class CFile : public Framework::CStream
	{
	public:

							CFile(CISO9660*, uint64, uint64);
							~CFile();
		void				Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
		uint64				Tell() override;
		uint64				Read(void*, uint64) override;
		uint64				Write(const void*, uint64) override;
		bool				IsEOF() override;

	private:
		void				SyncBlock();

		CISO9660*			m_iso;
		uint64				m_start;
		uint64				m_end;
		uint64				m_position;
		uint32				m_blockPosition;
		uint8				m_block[CISO9660::BLOCKSIZE];
	};

}
