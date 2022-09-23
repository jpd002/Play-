#pragma once

#include <vector>
#include <map>
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
	typedef std::vector<uint8> Cluster;
	typedef std::map<uint32, Cluster> ClusterMap;

	void ReadCluster(uint32, void*);
	void ReadClusterCached(uint32, void*);

	class CFatReader
	{
	public:
		CFatReader(CMcDumpReader&, uint32);

		uint32 Read(void*, uint32);

	private:
		enum
		{
			MC_PAGE_SIZE = 0x200,
			MC_PAGES_PER_CLUSTER = 2,
			MC_CLUSTER_SIZE = MC_PAGE_SIZE * MC_PAGES_PER_CLUSTER,
			INVALID_CLUSTER_IDX = -1,
		};

		uint32 GetNextFatClusterEntry(uint32);
		void ReadFatCluster(uint32);

		CMcDumpReader& m_parent;
		uint32 m_cluster = 0;
		uint32 m_bufferIndex = 0;
		uint8 m_buffer[MC_CLUSTER_SIZE];
	};

	Framework::CStream& m_stream;
	HEADER m_header = {};
	uint32 m_rawPageSize = 0;
	ClusterMap m_clusterCache;
};
