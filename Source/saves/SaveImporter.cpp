#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "SaveImporter.h"
#include "StdStreamUtils.h"

namespace filesystem = boost::filesystem;

CSaveImporter::CSaveImporter(const OverwritePromptFunctionType& OverwritePromptFunction)
{
	m_OverwritePromptFunction = OverwritePromptFunction;
	m_nOverwriteAll = false;
}

CSaveImporter::~CSaveImporter()
{

}

void CSaveImporter::ImportSave(Framework::CStream& input, const boost::filesystem::path& outputPath, const OverwritePromptFunctionType& OverwritePromptFunction)
{
	CSaveImporter SaveImporter(OverwritePromptFunction);
	SaveImporter.Import(input, outputPath);
}

void CSaveImporter::Import(Framework::CStream& input, const boost::filesystem::path& outputPath)
{
	uint32 nSignature = input.Read32();
	input.Seek(0, Framework::STREAM_SEEK_SET);

	if(nSignature == 0x00008427)
	{
		PSU_Import(input, outputPath);
	}
	else if(nSignature == 0x0000000D)
	{
		XPS_Import(input, outputPath);
	}
	else
	{
		throw std::runtime_error("Unknown input file format.");
	}
}

bool CSaveImporter::CanExtractFile(const filesystem::path& Path)
{
	if(!filesystem::exists(Path)) return true;
	if(m_nOverwriteAll) return true;

	OVERWRITE_PROMPT_RETURN nReturn = m_OverwritePromptFunction(filesystem::absolute(Path).string());

	switch(nReturn)
	{
	case OVERWRITE_YESTOALL:
		m_nOverwriteAll = true;
	case OVERWRITE_YES:
		return true;
		break;
	case OVERWRITE_NO:
		return false;
		break;
	}
	
	return false;
}

void CSaveImporter::XPS_Import(Framework::CStream& input, const boost::filesystem::path& outputPath)
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

	XPS_ExtractFiles(input, outputPath, nArchiveSize);
}

void CSaveImporter::XPS_ExtractFiles(Framework::CStream& input, const boost::filesystem::path& basePath, uint32 nArchiveSize)
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
			XPS_ExtractFiles(input, outputPath, nArchiveSize);
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

void CSaveImporter::PSU_Import(Framework::CStream& input, const boost::filesystem::path& basePath)
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
				PSU_Import(input, (basePath / sEntryName));

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
