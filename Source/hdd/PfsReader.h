#pragma once

#include "Stream.h"
#include "PfsDefs.h"
#include "ApaDefs.h"

namespace Hdd
{
	class CPfsReader
	{
	public:
		CPfsReader(Framework::CStream&, const APA_HEADER&);
		
		class CPfsFileReader* GetFileStream(const char*);

		uint32 GetZoneSize() const;
		uint32 GetBlockLba(uint32, uint32) const;

	private:
		PFS_INODE ReadInode(uint32, uint32);
		
		Framework::CStream& m_stream;

		APA_HEADER m_partitionHeader = {};
		PFS_SUPERBLOCK m_superBlock = {};
		uint32 m_inodeScale = 0;
	};

	class CPfsFileReader : public Framework::CStream
	{
	public:
		CPfsFileReader(CPfsReader&, Framework::CStream&, PFS_INODE);
		~CPfsFileReader() = default;
		
		void Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
		uint64 Tell() override;
		uint64 Read(void*, uint64) override;
		uint64 Write(const void*, uint64) override;
		bool IsEOF() override;

	private:
		CPfsReader& m_reader;
		Framework::CStream& m_stream;
		PFS_INODE m_inode;
		
		uint64 m_position = 0;
		bool m_isEof = false;
	};
}
