#pragma once

#include "Types.h"

namespace Iop
{
	namespace Ioman
	{
		enum STAT_MODE
		{
			// Directories have "group read" only permissions? This is required by PS2PSXe.
			STAT_MODE_DIR = (0747 | (1 << 12)),  // File mode + Dir type (1)
			STAT_MODE_FILE = (0777 | (2 << 12)), // File mode + File type (2)
		};

		struct STAT
		{
			uint32 mode;
			uint32 attr;
			uint32 loSize;
			uint8 creationTime[8];
			uint8 lastAccessTime[8];
			uint8 lastModificationTime[8];
			uint32 hiSize;
		};
		static_assert(sizeof(STAT) == 40, "STAT structure must be 40 bytes long.");

		//Same as above, except for added reserved fields (used in later versions of IOMAN)
		struct STATEX : public STAT
		{
			uint32 reserved[6];
		};
		static_assert(sizeof(STATEX) == 64, "STATEX structure must be 64 bytes long.");

		struct DIRENTRY
		{
			enum
			{
				NAME_SIZE = 256,
			};

			STAT stat;
			char name[NAME_SIZE];
			uint32 privatePtr;
		};
		static_assert(sizeof(DIRENTRY) == 0x12C, "DIRENTRY structure must be 300 bytes long");

		struct DEVICE
		{
			uint32 namePtr;
			int32 type;
			uint32 version;
			uint32 descPtr;
			uint32 opsPtr;
		};
		static_assert(sizeof(DEVICE) == 0x14, "Size of DEVICE must be 20 bytes.");

		struct DEVICEOPS
		{
			uint32 initPtr;
			uint32 deinitPtr;
			uint32 formatPtr;
			uint32 openPtr;
			uint32 closePtr;
			uint32 readPtr;
			uint32 writePtr;
			uint32 lseekPtr;
		};

		struct DEVICEFILE
		{
			uint32 mode;
			uint32 unit;
			uint32 devicePtr;
			uint32 privateData;
		};
		static_assert(sizeof(DEVICEFILE) == 0x10, "Size of DEVICE must be 16 bytes.");
	}
};
