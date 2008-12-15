#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include "SaveImporter.h"

using namespace std;
namespace iostreams = boost::iostreams;
namespace filesystem = boost::filesystem;

CSaveImporter::CSaveImporter(OverwritePromptFunctionType OverwritePromptFunction)
{
	m_OverwritePromptFunction = OverwritePromptFunction;
	m_nOverwriteAll = false;
}

CSaveImporter::~CSaveImporter()
{

}

void CSaveImporter::ImportSave(istream& Input, const char* sOutputPath, OverwritePromptFunctionType OverwritePromptFunction)
{
	CSaveImporter SaveImporter(OverwritePromptFunction);
	SaveImporter.Import(Input, sOutputPath);
}

void CSaveImporter::Import(istream& Input, const char* sOutputPath)
{
	uint32 nSignature;

	Input.read(reinterpret_cast<char*>(&nSignature), 4);
	Input.seekg(0, ios::beg);

	if(nSignature == 0x00008427)
	{
		PSU_Import(Input, sOutputPath);
	}
	else if(nSignature == 0x0000000D)
	{
		XPS_Import(Input, sOutputPath);
	}
	else
	{
		throw exception("Unknown input file format.");
	}
}

bool CSaveImporter::CanExtractFile(const filesystem::path& Path)
{
	if(!filesystem::exists(Path)) return true;
	if(m_nOverwriteAll) return true;

	OVERWRITE_PROMPT_RETURN nReturn;

	nReturn = m_OverwritePromptFunction(filesystem::complete(Path).string());

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

void CSaveImporter::XPS_Import(istream& Input, const char* sOutputPath)
{
	char sHeaderSig[0x0E];
	uint32 nStringLength, nArchiveSize;

	Input.seekg(4);

	Input.read(sHeaderSig, 0x0D);
	sHeaderSig[0x0D] = 0;

	if(strcmp(sHeaderSig, "SharkPortSave"))
	{
		throw exception("Invalid X-Port save file.");
	}

	Input.seekg(0x15);

	//Skip file title
	Input.read(reinterpret_cast<char*>(&nStringLength), 4);
	Input.seekg(nStringLength, ios::cur);

	//Skip file date
	Input.read(reinterpret_cast<char*>(&nStringLength), 4);
	Input.seekg(nStringLength, ios::cur);

	//Skip comment?
	Input.read(reinterpret_cast<char*>(&nStringLength), 4);
	Input.seekg(nStringLength, ios::cur);

	Input.read(reinterpret_cast<char*>(&nArchiveSize), 4);
	nArchiveSize += Input.tellg();

	XPS_ExtractFiles(Input, sOutputPath, nArchiveSize);
}

void CSaveImporter::XPS_ExtractFiles(istream& Input, const char* sOutputPath, uint32 nArchiveSize)
{
	filesystem::path Path(sOutputPath, filesystem::native);

	if(!filesystem::exists(Path))
	{
		filesystem::create_directory(Path);
	}

	while(!Input.eof() && ((uint32)Input.tellg() < nArchiveSize))
	{
		uint32 nLength, nAttributes;
		uint16 nDescriptorLength;
		char sName[0x40];

		Input.read(reinterpret_cast<char*>(&nDescriptorLength), 2);

		Input.read(sName, 0x40);
		Input.read(reinterpret_cast<char*>(&nLength), 4);

		Input.seekg(8, ios::cur);

		Input.read(reinterpret_cast<char*>(&nAttributes), 4);

		Input.seekg(nDescriptorLength - (0x40 + 4 + 4 + 8 + 2), ios::cur);

		if(nAttributes & 0x2000)
		{
			XPS_ExtractFiles(Input, (Path / sName).string().c_str(), nArchiveSize);
		}
		else
		{
			filesystem::path OutputPath;

			OutputPath = Path / sName;
			if(!CanExtractFile(OutputPath))
			{
				Input.seekg(nLength, ios::cur);
			}
			else
			{
				char sBuffer[1024];

				iostreams::filtering_ostream Output;
				Output.push(iostreams::file_sink(OutputPath.string(), ios::out | ios::binary));

				while(nLength != 0)
				{
					unsigned int nRead;

					nRead = min<unsigned int>(nLength, 1024);
					Input.read(sBuffer, nRead);
					Output.write(sBuffer, nRead);

					nLength -= nRead;
				}
			}
		}
	}
}

void CSaveImporter::PSU_Import(istream& Input, const char* sOutputPath)
{
	filesystem::path Path(sOutputPath, filesystem::native);
	uint16 nEntryType;

	if(!filesystem::exists(Path))
	{
		filesystem::create_directory(Path);
	}

	Input.read(reinterpret_cast<char*>(&nEntryType), 2);

	while(!Input.eof())
	{
		uint32 nEntrySize;
		char sEntryName[0x1C0];

		Input.seekg(2, ios::cur);
		Input.read(reinterpret_cast<char*>(&nEntrySize), 4);
		Input.seekg(0x38, ios::cur);
		Input.read(sEntryName, 0x1C0);

		if(nEntryType == 0x8427)
		{
			if(nEntrySize != 0)
			{
				PSU_Import(Input, (Path / sEntryName).string().c_str());
			}
		}
		else if(nEntryType == 0x8497)
		{
			filesystem::path OutputPath;

			OutputPath = Path / sEntryName;
			if(!CanExtractFile(OutputPath))
			{
				Input.seekg(nEntrySize, ios::cur);
			}
			else
			{
				unsigned int nEntrySizeDrain;

				iostreams::filtering_ostream Output;
				Output.push(iostreams::file_sink(OutputPath.string(), ios::out | ios::binary));

				nEntrySizeDrain = nEntrySize;

				while(nEntrySizeDrain != 0)
				{
					unsigned int nRead;
					char sBuffer[1024];

					nRead = min<unsigned int>(nEntrySizeDrain, 1024);
					Input.read(sBuffer, nRead);
					Output.write(sBuffer, nRead);

					nEntrySizeDrain -= nRead;
				}
			}

			Input.seekg(0x400 - (nEntrySize & 0x3FF), ios::cur);
		}
		else
		{
			throw exception("Invalid entry type encountered.");
		}

		Input.read(reinterpret_cast<char*>(&nEntryType), 2);
	}
}
