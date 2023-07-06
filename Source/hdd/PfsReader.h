#pragma once

#include "Stream.h"
#include "HddDefs.h"
#include "ApaDefs.h"
#include "PfsDefs.h"

namespace Hdd
{
	class CPfsReader
	{
	public:
		CPfsReader(Framework::CStream&, const APA_HEADER&);

		class CPfsFileReader* GetFileStream(const char*);
		class CPfsDirectoryReader* GetDirectoryReader(const char*);

		uint32 GetZoneSize() const;
		uint32 GetBlockLba(uint32, uint32) const;

		PFS_INODE ReadInode(uint32, uint32);

	private:
		bool TryGetInodeFromPath(const char*, PFS_INODE&);

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

	class CPfsDirectoryReader
	{
	public:
		CPfsDirectoryReader(CPfsReader&, Framework::CStream&, PFS_INODE);

		void ReadEntry(std::string&, PFS_INODE&);
		bool IsDone() const;

	private:
		static const uint32_t g_dirBlockSize = Hdd::g_sectorSize << PFS_BLOCK_SCALE;

		void Advance();

		CPfsReader& m_reader;

		uint8 m_dirBlock[g_dirBlockSize];
		uint8* m_dirBlockCurr = nullptr;
		uint8* m_dirBlockEnd = nullptr;

		std::string m_currentEntryName;
		PFS_INODE m_currentEntryInode;
	};
}
