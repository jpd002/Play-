#include <boost/filesystem/operations.hpp>
#include "SaveExporter.h"
#include "StdStream.h"
#include "StdStreamUtils.h"

void CSaveExporter::ExportPSU(Framework::CStream& outputStream, const boost::filesystem::path& savePath)
{
	uint32 nSector = 0x18D;

	boost::posix_time::ptime baseTime = 
		boost::posix_time::from_time_t(boost::filesystem::last_write_time(savePath));

	//Save base directory entry
	{
		PSUENTRY Entry;
		memset(&Entry, 0, sizeof(PSUENTRY));

		Entry.nSize = 2;

		const boost::filesystem::directory_iterator itEnd;
		for(boost::filesystem::directory_iterator itElement(savePath);
			itElement != itEnd;
			itElement++)
		{
			if(boost::filesystem::is_directory(*itElement)) continue;
			Entry.nSize++;
		}

		Entry.nFlags					= 0x8427;
		Entry.nSector					= nSector;

		PSU_CopyTime(&Entry.CreationTime,		baseTime);
		PSU_CopyTime(&Entry.ModificationTime,	baseTime);

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

		Entry.nFlags					= 0x8427;
		Entry.nSector					= 0;

		PSU_CopyTime(&Entry.CreationTime,		baseTime);
		PSU_CopyTime(&Entry.ModificationTime,	baseTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), sRelDirName[i], 0x1C0);

		outputStream.Write(&Entry, sizeof(PSUENTRY));
	}

	nSector += 2;

	//Save individual file entries
	const boost::filesystem::directory_iterator itEnd;
	for(boost::filesystem::directory_iterator itElement(savePath);
		itElement != itEnd;
		itElement++)
	{
		if(boost::filesystem::is_directory(*itElement)) continue;

		boost::filesystem::path saveItemPath(*itElement);

		boost::posix_time::ptime itemTime = 
			boost::posix_time::from_time_t(boost::filesystem::last_write_time(saveItemPath));

		PSUENTRY Entry;
		memset(&Entry, 0, sizeof(PSUENTRY));

		Entry.nFlags					= 0x8497;
		Entry.nSize						= static_cast<uint32>(boost::filesystem::file_size(saveItemPath));
		Entry.nSector					= nSector;

		PSU_CopyTime(&Entry.CreationTime,		itemTime);
		PSU_CopyTime(&Entry.ModificationTime,	itemTime);

		strncpy(reinterpret_cast<char*>(Entry.sName), saveItemPath.filename().string().c_str(), 0x1C0);

		outputStream.Write(&Entry, sizeof(PSUENTRY));

		//Write file contents
		{
			boost::scoped_ptr<Framework::CStdStream> itemStream(Framework::CreateInputStdStream(saveItemPath.native()));
			uint32 nSize = Entry.nSize;

			while(nSize != 0)
			{
				const uint32 bufferSize = 0x1000;
				char sBuffer[bufferSize];

				uint32 nRead = std::min<uint32>(nSize, bufferSize);
				itemStream->Read(sBuffer, nRead);
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

void CSaveExporter::PSU_CopyTime(PSUENTRY::TIME* pDst, const boost::posix_time::ptime& pSrc)
{
	pDst->nSecond	= static_cast<uint8>(pSrc.time_of_day().seconds());
	pDst->nMinute	= static_cast<uint8>(pSrc.time_of_day().minutes());
	pDst->nHour		= static_cast<uint8>(pSrc.time_of_day().hours());
	pDst->nDay		= static_cast<uint8>(pSrc.date().day());
	pDst->nMonth	= static_cast<uint8>(pSrc.date().month());
	pDst->nYear		= pSrc.date().year();
}
