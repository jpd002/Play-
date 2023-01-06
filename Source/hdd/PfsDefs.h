#pragma once

namespace Hdd
{
	enum
	{
		PFS_SUPERBLOCK_MAGIC = 0x50465300,           //'/0sfp'
		PFS_INODE_SEGDESC_DIRECT_MAGIC = 0x53454744, //'dges'

		PFS_SUPERBLOCK_LBA = 8192,

		PFS_INODE_SIZE = 1024,
		PFS_BLOCK_SCALE = 1,
	};

	struct PFS_BLOCKINFO
	{
		uint32_t number;
		uint16_t subPart;
		uint16_t count;
	};
	static_assert(sizeof(PFS_BLOCKINFO) == 0x8);

	struct PFS_TIME
	{
		uint8_t unused;
		uint8_t seconds;
		uint8_t minutes;
		uint8_t hours;
		uint8_t day;
		uint8_t month;
		uint16_t year;
	};
	static_assert(sizeof(PFS_TIME) == 8);

	struct PFS_SUPERBLOCK
	{
		uint32_t magic;
		uint32_t version;
		uint32_t modVer;
		uint32_t fsckStat;
		uint32_t zoneSize;
		uint32_t subPartCount;
		PFS_BLOCKINFO logBlock;
		PFS_BLOCKINFO rootBlock;
	};
	static_assert(sizeof(PFS_SUPERBLOCK) == 0x28);

	struct PFS_INODE
	{
		enum
		{
			DATA_SIZE = 114,
		};

		uint32_t checksum;
		uint32_t magic;
		PFS_BLOCKINFO inodeBlock;
		PFS_BLOCKINFO nextSegment;
		PFS_BLOCKINFO lastSegment;
		PFS_BLOCKINFO unused;
		PFS_BLOCKINFO data[DATA_SIZE];
		uint16_t mode;
		uint16_t attr;
		uint16_t uid;
		uint16_t gid;
		PFS_TIME atime;
		PFS_TIME ctime;
		PFS_TIME mtime;
		uint64_t size;
		uint32_t blockCount;
		uint32_t dataCount;
		uint32_t indirectCount;
		uint32_t subPart;
		uint32_t reserved[4];
	};
	static_assert(sizeof(PFS_INODE) == 1024);

	struct PFS_DIRENTRY
	{
		uint32_t inode;
		uint8_t subPart;
		uint8_t pathLength;
		uint16_t allocatedLength;
	};
	static_assert(sizeof(PFS_DIRENTRY) == 8);
}
