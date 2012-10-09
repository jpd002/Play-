#include "XpsSaveImporter.h"
#include "StdStreamUtils.h"

namespace filesystem = boost::filesystem;

CXpsSaveImporter::CXpsSaveImporter()
{

}

CXpsSaveImporter::~CXpsSaveImporter()
{

}

void CXpsSaveImporter::Import(Framework::CStream& input, const boost::filesystem::path& outputPath)
{
	input.Seek(4, Framework::STREAM_SEEK_SET);

	{
		char sHeaderSig[0x0E];
		input.Read(sHeaderSig, 0x0D);
		sHeaderSig[0x0D] = 0;

		if(strcmp(sHeaderSig, "SharkPortSave"))
		{
			throw std::runtime_error("Invalid X-Port save file.");
		}
	}

	input.Seek(0x15, Framework::STREAM_SEEK_SET);

	//Skip file title
	{
		uint32 nStringLength = input.Read32();
		input.Seek(nStringLength, Framework::STREAM_SEEK_CUR);
	}

	//Skip file date
	{
		uint32 nStringLength = input.Read32();
		input.Seek(nStringLength, Framework::STREAM_SEEK_CUR);
	}

	//Skip comment?
	{
		uint32 nStringLength = input.Read32();
		input.Seek(nStringLength, Framework::STREAM_SEEK_CUR);
	}

	uint32 nArchiveSize = input.Read32();
	nArchiveSize += static_cast<uint32>(input.Tell());

	ExtractFiles(input, outputPath, nArchiveSize);
}

void CXpsSaveImporter::ExtractFiles(Framework::CStream& input, const boost::filesystem::path& basePath, uint32 nArchiveSize)
{
	if(!filesystem::exists(basePath))
	{
		filesystem::create_directory(basePath);
	}

	while(!input.IsEOF() && (static_cast<uint32>(input.Tell()) < nArchiveSize))
	{
		uint16 nDescriptorLength = input.Read16();

		char sName[0x41];
		memset(sName, 0, sizeof(sName));
		input.Read(sName, 0x40);

		uint32 nLength = input.Read32();
		input.Seek(8, Framework::STREAM_SEEK_CUR);
		uint32 nAttributes = input.Read32();

		input.Seek(nDescriptorLength - (0x40 + 4 + 4 + 8 + 2), Framework::STREAM_SEEK_CUR);

		filesystem::path outputPath(basePath / sName);

		if(nAttributes & 0x2000)
		{
			ExtractFiles(input, outputPath, nArchiveSize);
		}
		else
		{
			if(!CanExtractFile(outputPath))
			{
				input.Seek(nLength, Framework::STREAM_SEEK_CUR);
			}
			else
			{
				auto output(Framework::CreateOutputStdStream(outputPath.native()));

				while(nLength != 0)
				{
					const int bufferSize = 1024;
					char sBuffer[bufferSize];

					unsigned int nRead = std::min<unsigned int>(nLength, bufferSize);
					input.Read(sBuffer, nRead);
					output.Write(sBuffer, nRead);

					nLength -= nRead;
				}
			}
		}
	}
}
