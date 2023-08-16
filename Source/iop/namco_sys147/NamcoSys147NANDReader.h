#pragma once

#include <vector>
#include "Stream.h"

namespace Namco
{
	class CSys147NANDReader
	{
	public:
#pragma pack(push, 1)
		struct DIRHEADER
		{
			char signature[8];
			char unknown0[8];
			uint32 entryCount;
			uint32 unknown1;
			uint32 unknown2;
			uint32 unknown3;
		};
		static_assert(sizeof(DIRHEADER) == 32);

		struct DIRENTRY
		{
			char name[0x14];
			uint32 type;
			uint32 size;
			uint32 sector;
		};
		static_assert(sizeof(DIRENTRY) == 32);
#pragma pack(pop)

		typedef std::vector<DIRENTRY> Directory;

		CSys147NANDReader(Framework::CStream&, uint32);
		
		Directory ReadDirectory(uint32);
		std::vector<uint8> ReadFile(uint32, uint32);
		
	private:
		void ReadSector(uint32, void*);
		
		static constexpr uint32 m_sectorSize = 0x840;
		static constexpr uint32 m_dataSize = 0x800;
		static constexpr uint32 m_eccSize = 0x40;

		Framework::CStream& m_stream;
		uint32 m_baseSector = 0;
	};
}
