#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include "SaveExporter.h"
#include "Types.h"

using namespace std;
using namespace boost;

void CSaveExporter::ExportPSU(ostream& Output, const char* sSavePath)
{
	filesystem::path SavePath(sSavePath, filesystem::native);
	filesystem::directory_iterator itEnd;

	uint32 nSector = 0x18D;

#pragma pack(push, 1)

	struct ENTRY
	{
		//All fields seem to be BCDs except for year
		struct TIME
		{
			uint8	nUnknown;
			uint8	nSecond;
			uint8	nMinute;
			uint8	nHour;
			uint8	nDay;
			uint8	nMonth;
			uint16	nYear;
		};

		uint32	nFlags;
		uint32	nSize;
		TIME	CreationTime;
		uint32	nSector;
		uint32	nChecksum;
		TIME	ModificationTime;
		uint8	Padding[0x20];
		uint8	sName[0x1C0];
	};

#pragma pack(pop)

	assert(sizeof(ENTRY) == 0x200);

	{
		//Save base directory entry

		ENTRY Entry;

		memset(&Entry, 0, sizeof(ENTRY));

		Entry.nSize = 2;

		for(filesystem::directory_iterator itElement(SavePath);
			itElement != itEnd;
			itElement++)
		{
			if(filesystem::is_directory(*itElement)) continue;
			Entry.nSize++;
		}

		Entry.nFlags					= 0x8427;
		Entry.nSector					= nSector;

		Entry.CreationTime.nSecond		= 0x27;
		Entry.CreationTime.nMinute		= 0x23;
		Entry.CreationTime.nHour		= 0x11;
		Entry.CreationTime.nDay			= 0x17;
		Entry.CreationTime.nMonth		= 0x06;
		Entry.CreationTime.nYear		= 2006;

		Entry.ModificationTime.nSecond	= 0x27;
		Entry.ModificationTime.nMinute	= 0x23;
		Entry.ModificationTime.nHour	= 0x11;
		Entry.ModificationTime.nDay		= 0x17;
		Entry.ModificationTime.nMonth	= 0x06;
		Entry.ModificationTime.nYear	= 2006;

		strncpy(reinterpret_cast<char*>(Entry.sName), SavePath.leaf().c_str(), 0x1C0);

		Output.write(reinterpret_cast<char*>(&Entry), sizeof(ENTRY));
	}

	static const char* sRelDirName[2] = 
	{
		".",
		"..",
	};

	//Save "relative" directories

	for(unsigned int i = 0; i < 2; i++)
	{
		ENTRY Entry;

		memset(&Entry, 0, sizeof(ENTRY));

		Entry.nFlags					= 0x8427;
		Entry.nSector					= 0;

		Entry.CreationTime.nSecond		= 0x27;
		Entry.CreationTime.nMinute		= 0x23;
		Entry.CreationTime.nHour		= 0x11;
		Entry.CreationTime.nDay			= 0x17;
		Entry.CreationTime.nMonth		= 0x06;
		Entry.CreationTime.nYear		= 2006;

		Entry.ModificationTime.nSecond	= 0x27;
		Entry.ModificationTime.nMinute	= 0x23;
		Entry.ModificationTime.nHour	= 0x11;
		Entry.ModificationTime.nDay		= 0x17;
		Entry.ModificationTime.nMonth	= 0x06;
		Entry.ModificationTime.nYear	= 2006;

		strncpy(reinterpret_cast<char*>(Entry.sName), sRelDirName[i], 0x1C0);

		Output.write(reinterpret_cast<char*>(&Entry), sizeof(ENTRY));
	}

	nSector += 2;

	//Save individual file entries
	for(filesystem::directory_iterator itElement(SavePath);
		itElement != itEnd;
		itElement++)
	{
		if(filesystem::is_directory(*itElement)) continue;

		ENTRY Entry;
		uint32 nSize;

		memset(&Entry, 0, sizeof(ENTRY));

		Entry.nFlags					= 0x8497;
		Entry.nSize						= static_cast<uint32>(filesystem::file_size(*itElement));
		Entry.nSector					= nSector;

		Entry.CreationTime.nSecond		= 0x27;
		Entry.CreationTime.nMinute		= 0x23;
		Entry.CreationTime.nHour		= 0x11;
		Entry.CreationTime.nDay			= 0x17;
		Entry.CreationTime.nMonth		= 0x06;
		Entry.CreationTime.nYear		= 2006;

		Entry.ModificationTime.nSecond	= 0x27;
		Entry.ModificationTime.nMinute	= 0x23;
		Entry.ModificationTime.nHour	= 0x11;
		Entry.ModificationTime.nDay		= 0x17;
		Entry.ModificationTime.nMonth	= 0x06;
		Entry.ModificationTime.nYear	= 2006;

		strncpy(reinterpret_cast<char*>(Entry.sName), (*itElement).leaf().c_str(), 0x1C0);

		Output.write(reinterpret_cast<char*>(&Entry), sizeof(ENTRY));

		//Write file contents
		{
			ifstream Input((*itElement).string().c_str(), ios::in | ios::binary);
			nSize = Entry.nSize;

			while(nSize != 0)
			{
				uint32 nRead;
				char sBuffer[0x1000];

				nRead = min<uint32>(nSize, 0x1000);
				Input.read(sBuffer, nRead);
				Output.write(sBuffer, nRead);
				nSize -= nRead;
			}

			Input.close();
		}

		//Align on 0x400 boundary
		{
			unsigned int nPosition;
			char sBuffer[0x400];

			nPosition = Entry.nSize;
			memset(sBuffer, 0, 0x400);
			
			Output.write(sBuffer, 0x400 - (nPosition & 0x3FF));
		}

		nSector += (Entry.nSize + 0x3FF) / 0x400;
	}
}
