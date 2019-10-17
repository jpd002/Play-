#include <cstring>
#include "SaveExporter.h"
#include "StdStream.h"
#include "StdStreamUtils.h"
#include "FilesystemUtils.h"

void CSaveExporter::ExportPSU(Framework::CStream& outputStream, const fs::path& savePath)
{
	uint32 nSector = 0x18D;

	auto baseTime = fs::last_write_time(savePath);

	//Save base directory entry
	{
		PSUENTRY Entry;
		memset(&Entry, 0, sizeof(PSUENTRY));

		Entry.nSize = 2;

		const fs::directory_iterator itEnd;
		for(fs::directory_iterator itElement(savePath);
		    itElement != itEnd;
		    itElement++)
		{
			if(fs::is_directory(*itElement)) continue;
			Entry.nSize++;
		}

		Entry.nFlags = 0x8427;
		Entry.nSector = nSector;

		PSU_CopyTime(&Entry.CreationTime, baseTime);
		PSU_CopyTime(&Entry.ModificationTime, baseTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), savePath.filename().string().c_str(), 0x1C0);

		outputStream.Write(&Entry, sizeof(PSUENTRY));
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

		Entry.nFlags = 0x8427;
		Entry.nSector = 0;

		PSU_CopyTime(&Entry.CreationTime, baseTime);
		PSU_CopyTime(&Entry.ModificationTime, baseTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), sRelDirName[i], 0x1C0);

		outputStream.Write(&Entry, sizeof(PSUENTRY));
	}

	nSector += 2;

	//Save individual file entries
	const fs::directory_iterator itEnd;
	for(fs::directory_iterator itElement(savePath);
	    itElement != itEnd;
	    itElement++)
	{
		if(fs::is_directory(*itElement)) continue;

		fs::path saveItemPath(*itElement);

		auto itemTime = fs::last_write_time(saveItemPath);

		PSUENTRY Entry;
		memset(&Entry, 0, sizeof(PSUENTRY));

		Entry.nFlags = 0x8497;
		Entry.nSize = static_cast<uint32>(fs::file_size(saveItemPath));
		Entry.nSector = nSector;

		PSU_CopyTime(&Entry.CreationTime, itemTime);
		PSU_CopyTime(&Entry.ModificationTime, itemTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), saveItemPath.filename().string().c_str(), 0x1C0);

		outputStream.Write(&Entry, sizeof(PSUENTRY));

		//Write file contents
		{
			auto itemStream(Framework::CreateInputStdStream(saveItemPath.native()));
			uint32 nSize = Entry.nSize;

			while(nSize != 0)
			{
				const uint32 bufferSize = 0x1000;
				char sBuffer[bufferSize];

				uint32 nRead = std::min<uint32>(nSize, bufferSize);
				itemStream.Read(sBuffer, nRead);
				outputStream.Write(sBuffer, nRead);
				nSize -= nRead;
			}
		}

		//Align on 0x400 boundary
		{
			char sBuffer[0x400];

			unsigned int nPosition = Entry.nSize;
			memset(sBuffer, 0, 0x400);

			outputStream.Write(sBuffer, 0x400 - (nPosition & 0x3FF));
		}

		nSector += (Entry.nSize + 0x3FF) / 0x400;
	}
}

void CSaveExporter::PSU_CopyTime(PSUENTRY::TIME* pDst, const fs::file_time_type& fileTime)
{
	auto systemTime = Framework::ConvertFsTimeToSystemTime(fileTime);
	const auto pSrc = std::localtime(&systemTime);
	pDst->nSecond = static_cast<uint8>(pSrc->tm_sec);
	pDst->nMinute = static_cast<uint8>(pSrc->tm_min);
	pDst->nHour = static_cast<uint8>(pSrc->tm_hour);
	pDst->nDay = static_cast<uint8>(pSrc->tm_mday);
	pDst->nMonth = static_cast<uint8>(pSrc->tm_mon);
	pDst->nYear = pSrc->tm_year;
}
