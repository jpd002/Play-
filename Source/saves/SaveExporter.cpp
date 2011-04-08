#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include "SaveExporter.h"

using namespace std;
using namespace boost;

void CSaveExporter::ExportPSU(ostream& Output, const char* sSavePath)
{
	filesystem::path SavePath(sSavePath, filesystem::native);
	filesystem::directory_iterator itEnd;
	posix_time::ptime BaseTime;

	uint32 nSector = 0x18D;

	assert(sizeof(PSUENTRY) == 0x200);

	BaseTime = posix_time::from_time_t(filesystem::last_write_time(SavePath));

	{
		//Save base directory entry

		PSUENTRY Entry;

		memset(&Entry, 0, sizeof(PSUENTRY));

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

		PSU_CopyTime(&Entry.CreationTime,		&BaseTime);
		PSU_CopyTime(&Entry.ModificationTime,	&BaseTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), SavePath.filename().string().c_str(), 0x1C0);

		Output.write(reinterpret_cast<char*>(&Entry), sizeof(PSUENTRY));
	}

	static const char* sRelDirName[2] = 
	{
		".",
		"..",
	};

	//Save "relative" directories

	for(unsigned int i = 0; i < 2; i++)
	{
		PSUENTRY Entry;

		memset(&Entry, 0, sizeof(PSUENTRY));

		Entry.nFlags					= 0x8427;
		Entry.nSector					= 0;

		PSU_CopyTime(&Entry.CreationTime,		&BaseTime);
		PSU_CopyTime(&Entry.ModificationTime,	&BaseTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), sRelDirName[i], 0x1C0);

		Output.write(reinterpret_cast<char*>(&Entry), sizeof(PSUENTRY));
	}

	nSector += 2;

	//Save individual file entries
	for(filesystem::directory_iterator itElement(SavePath);
		itElement != itEnd;
		itElement++)
	{
		if(filesystem::is_directory(*itElement)) continue;

		boost::filesystem::path savePath(*itElement);

		PSUENTRY Entry;
		uint32 nSize;

		posix_time::ptime FileTime = posix_time::from_time_t(filesystem::last_write_time(savePath));

		memset(&Entry, 0, sizeof(PSUENTRY));

		Entry.nFlags					= 0x8497;
		Entry.nSize						= static_cast<uint32>(filesystem::file_size(savePath));
		Entry.nSector					= nSector;

		PSU_CopyTime(&Entry.CreationTime,		&FileTime);
		PSU_CopyTime(&Entry.ModificationTime,	&FileTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), savePath.filename().string().c_str(), 0x1C0);

		Output.write(reinterpret_cast<char*>(&Entry), sizeof(PSUENTRY));

		//Write file contents
		{
			ifstream Input(savePath.string().c_str(), ios::in | ios::binary);
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

void CSaveExporter::PSU_CopyTime(PSUENTRY::TIME* pDst, posix_time::ptime* pSrc)
{
	pDst->nSecond	= static_cast<uint8>(pSrc->time_of_day().seconds());
	pDst->nMinute	= static_cast<uint8>(pSrc->time_of_day().minutes());
	pDst->nHour		= static_cast<uint8>(pSrc->time_of_day().hours());
	pDst->nDay		= static_cast<uint8>(pSrc->date().day());
	pDst->nMonth	= static_cast<uint8>(pSrc->date().month());
	pDst->nYear		= pSrc->date().year();
}
