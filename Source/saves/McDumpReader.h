#pragma once

#include <vector>
#include "Stream.h"

class CMcDumpReader
{
public:
	enum
	{
		DF_FILE = 0x10,
		DF_DIRECTORY = 0x20,
	};
	
#pragma pack(push, 1)
	struct HEADER
	{
		char magic[28];
		char version[12];
		uint16 pageSize;
		uint16 pagesPerCluster;
		uint16 pagesPerBlock;
		uint16 unknown0;
		uint32 clustersPerCard;
		uint32 allocOffset;
		uint32 allocEnd;
		uint32 rootDirCluster;
		uint32 backupBlock1;
		uint32 backupBlock2;
		uint32 unknown1[2];
		uint32 ifcList[32];
		uint32 badBlockList[32];
		uint8 cardType;
		uint8 cardFlags;
		uint8 padding[2];
	};
	static_assert(sizeof(HEADER) == 340);
	
	struct DIRENTRY
	{
		uint16 mode;
		uint16 unknown0;
		uint32 length;
		uint8 creationTime[8];
		uint32 cluster;
		uint32 dirEntry;
		uint8 modificationTime[8];
		uint32 attributes;
		uint32 unknown2[7];
		char name[0x20];
		uint8 padding[416];
	};
	static_assert(sizeof(DIRENTRY) == 512);
#pragma pack(pop)
	
	typedef std::vector<DIRENTRY> Directory;

	CMcDumpReader(Framework::CStream&);

	Directory ReadDirectory(uint32);
	Directory ReadRootDirectory();
	std::vector<uint8> ReadFile(uint32, uint32);

private:
	class CFatReader
	{
	public:
		CFatReader(Framework::CStream&, const HEADER&, uint32);
		
		uint32 Read(void*, uint32);
		
	private:
		enum
		{
			CLUSTER_SIZE = 0x400,
			INVALID_CLUSTER_IDX = -1,
		};
		
		uint32 GetNextFatClusterEntry(uint32);
		void ReadFatCluster(uint32);

		Framework::CStream& m_stream;
		const HEADER& m_header;
		uint32 m_cluster = 0;
		uint32 m_bufferIndex = 0;
		uint8 m_buffer[CLUSTER_SIZE];
	};
	
	Framework::CStream& m_stream;
	HEADER m_header = {};
};

