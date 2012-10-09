#include "PsuSaveImporter.h"
#include "StdStreamUtils.h"

namespace filesystem = boost::filesystem;

CPsuSaveImporter::CPsuSaveImporter()
{

}

CPsuSaveImporter::~CPsuSaveImporter()
{

}

void CPsuSaveImporter::Import(Framework::CStream& input, const filesystem::path& basePath)
{
	if(!filesystem::exists(basePath))
	{
		filesystem::create_directory(basePath);
	}

	uint16 nEntryType = input.Read16();

	while(!input.IsEOF())
	{
		char sEntryName[0x1C0];

		input.Seek(2, Framework::STREAM_SEEK_CUR);
		uint32 nEntrySize = input.Read32();
		input.Seek(0x38, Framework::STREAM_SEEK_CUR);
		input.Read(sEntryName, 0x1C0);

		if(nEntryType == 0x8427)
		{
			if(nEntrySize != 0)
			{
				Import(input, (basePath / sEntryName));

				if(input.IsEOF())
				{
					break;
				}
			}
		}
		else if(nEntryType == 0x8497)
		{
			filesystem::path outputPath = basePath / sEntryName;
			if(!CanExtractFile(outputPath))
			{
				input.Seek(nEntrySize, Framework::STREAM_SEEK_CUR);
			}
			else
			{
				auto output(Framework::CreateOutputStdStream(outputPath.native()));

				unsigned int nEntrySizeDrain = nEntrySize;

				while(nEntrySizeDrain != 0)
				{
					char sBuffer[1024];
					unsigned int nRead = std::min<unsigned int>(nEntrySizeDrain, 1024);

					input.Read(sBuffer, nRead);
					output.Write(sBuffer, nRead);

					nEntrySizeDrain -= nRead;
				}
			}

			input.Seek(0x400 - (nEntrySize & 0x3FF), Framework::STREAM_SEEK_CUR);
		}
		else
		{
			throw std::runtime_error("Invalid entry type encountered.");
		}

		nEntryType = input.Read16();
	}
}
