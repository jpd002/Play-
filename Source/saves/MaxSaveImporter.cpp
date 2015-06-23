#include <stdexcept>
#include "MaxSaveImporter.h"
#include "LzAri.h"
#include "MemStream.h"
#include "StdStreamUtils.h"

namespace filesystem = boost::filesystem;

CMaxSaveImporter::CMaxSaveImporter()
{

}

CMaxSaveImporter::~CMaxSaveImporter()
{

}

void CMaxSaveImporter::Import(Framework::CStream& inputStream, const filesystem::path& basePath)
{
	char magic[12];
	inputStream.Read(magic, sizeof(magic));
	if(memcmp(magic, "Ps2PowerSave", sizeof(magic)) != 0)
	{
		throw std::runtime_error("Invalid MAX save file.");
	}

	uint32 checksum = inputStream.Read32();

	char directoryName[0x21];
	inputStream.Read(directoryName, 0x20);
	directoryName[0x20] = 0;

	char iconName[0x21];
	inputStream.Read(iconName, 0x20);
	iconName[0x20] = 0;

	uint32 compressedSize = inputStream.Read32();
	uint32 fileCount = inputStream.Read32();

	Framework::CMemStream directoryDataStream;
	Framework::CLzAri::Decompress(directoryDataStream, inputStream);
	directoryDataStream.Seek(0, Framework::STREAM_SEEK_SET);

	auto directoryPath = basePath / directoryName;
	if(!filesystem::exists(directoryPath))
	{
		filesystem::create_directory(directoryPath);
	}

	for(unsigned int i = 0; i < fileCount; i++)
	{
		uint32 fileSize = directoryDataStream.Read32();

		char fileName[0x21];
		directoryDataStream.Read(fileName, 0x20);
		fileName[0x20] = 0;

		auto filePath = directoryPath / fileName;

		if(!CanExtractFile(filePath))
		{
			directoryDataStream.Seek(fileSize, Framework::STREAM_SEEK_CUR);
		}
		else
		{
			auto fileStream(Framework::CreateOutputStdStream(filePath.native()));

			while(fileSize != 0)
			{
				const int bufferSize = 1024;
				char buffer[bufferSize];

				unsigned int readAmount = std::min<unsigned int>(fileSize, bufferSize);
				directoryDataStream.Read(buffer, readAmount);
				fileStream.Write(buffer, readAmount);

				fileSize -= readAmount;
			}
		}

		//Align stream
		{
			auto currPos = directoryDataStream.Tell();
			auto padding = (((currPos + 8) + 15) & ~15) - 8 - currPos;
			directoryDataStream.Seek(padding, Framework::STREAM_SEEK_CUR);
		}
	}
}
