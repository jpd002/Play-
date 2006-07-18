#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include "SaveImporter.h"

using namespace std;
using namespace boost;

void CSaveImporter::ImportSave(istream& Input, const char* sOutputPath)
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
			char sBuffer[1024];

			iostreams::filtering_ostream Output;
			Output.push(iostreams::file_sink((Path / sName).string(), ios::out | ios::binary));

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

		if(nEntryType == 0x8427 && (nEntrySize != 0))
		{
			PSU_Import(Input, (Path / sEntryName).string().c_str());
		}
		else if(nEntryType == 0x8497)
		{
			char sBuffer[1024];

			iostreams::filtering_ostream Output;
			Output.push(iostreams::file_sink((Path / sEntryName).string(), ios::out | ios::binary));

			while(nEntrySize != 0)
			{
				unsigned int nRead;

				nRead = min<unsigned int>(nEntrySize, 1024);
				Input.read(sBuffer, nRead);
				Output.write(sBuffer, nRead);

				nEntrySize -= nRead;
			}

			unsigned int nPosition;
			nPosition = Input.tellg();
			Input.seekg(0x200 - (nPosition & 0x1FF), ios::cur);
		}

		Input.read(reinterpret_cast<char*>(&nEntryType), 2);
	}
}
