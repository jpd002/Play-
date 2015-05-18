#pragma once

#include "BlockProvider.h"

namespace ISO9660
{

	class CFile : public Framework::CStream
	{
	public:
							CFile(CBlockProvider*, uint64);
							CFile(CBlockProvider*, uint64, uint64);
							~CFile();
		void				Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
		uint64				Tell() override;
		uint64				Read(void*, uint64) override;
		uint64				Write(const void*, uint64) override;
		bool				IsEOF() override;

	private:
		void				InitBlock();
		void				SyncBlock();

		CBlockProvider*		m_blockProvider = nullptr;
		uint64				m_start = 0;
		uint64				m_end = 0;
		uint64				m_position = 0;
		uint32				m_blockPosition = 0;
		uint8				m_block[CBlockProvider::BLOCKSIZE];
		bool				m_isEof = false;
	};

}
