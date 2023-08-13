#pragma once

#include <cstdint>

namespace Hdd
{
	enum
	{
		APA_HEADER_MAGIC = 0x415041,
	};

	struct APA_TIME
	{
		uint8_t unused;
		uint8_t seconds;
		uint8_t minutes;
		uint8_t hours;
		uint8_t day;
		uint8_t month;
		uint16_t year;
	};
	static_assert(sizeof(APA_TIME) == 8);

	struct APA_MBR
	{
		char magic[0x20];
		uint32_t version;
		uint32_t sectorCount;
		APA_TIME createdTime;
		uint32_t osdStart;
		uint32_t osdSize;
		char padding[200];
	};
	static_assert(sizeof(APA_MBR) == 0x100);

	struct APA_SUBPART
	{
		uint32_t start;
		uint32_t count;
	};
	static_assert(sizeof(APA_SUBPART) == 8);

#pragma pack(push, 1)
	struct APA_HEADER
	{
		enum
		{
			ID_SIZE = 32,
			PASSWORD_SIZE = 8,
			SUBPART_SIZE = 64,
		};

		enum TYPE
		{
			TYPE_PFS = 0x100,
		};

		uint32_t checksum;
		uint32_t magic;
		uint32_t next;
		uint32_t prev;
		char id[ID_SIZE];
		char rPassword[PASSWORD_SIZE];
		char fPassword[PASSWORD_SIZE];
		uint32_t start;
		uint32_t length;
		uint16_t type;
		uint16_t flags;
		uint32_t subPartCount;
		APA_TIME createdTime;
		uint32_t main;
		uint32_t number;
		uint32_t modVer;
		uint32_t padding1[7];
		uint8_t padding2[0x80];
		APA_MBR mbr;
		APA_SUBPART subParts[SUBPART_SIZE];
	};
	static_assert(sizeof(APA_HEADER) == 0x400);
#pragma pack(pop)
}
