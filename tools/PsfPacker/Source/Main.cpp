#include <memory>
#include <vector>
#include <assert.h>
#include <boost/filesystem/operations.hpp>
#include "StdStream.h"
#include "PsfFsDescription.h"
#include "PsfFsWriter.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/Utils.h"

struct PSFHEADER
{
	uint8		signature[3];
	uint8		version;
	uint32		reservedSize;
	uint32		programSize;
	uint32		programCrc;
};
BOOST_STATIC_ASSERT(sizeof(PSFHEADER) == 0x10);

int main(int argc, const char** argv)
{
	if(argc < 2)
	{
		printf("Usage: PsfPacker [descriptorFile] [outputFile]\r\n");
		return -1;
	}

	PsfFs::DirectoryPtr rootDir;
	{
		std::tr1::shared_ptr<Framework::Xml::CNode> documentRoot;

		try
		{
			Framework::CStdStream stream(argv[1], "rb");
			Framework::Xml::CNode* result = Framework::Xml::CParser::ParseDocument(&stream);
			if(result == NULL)
			{
				throw std::exception("Cannot parse XML file.");
			}
			documentRoot = std::tr1::shared_ptr<Framework::Xml::CNode>(result);
		}
		catch(const std::exception& exception)
		{
			printf("Error. Couldn't process input file: %s.\r\n", exception.what());
			return -1;
		}

		{
			Framework::Xml::CNode* filesystemNode = documentRoot->Select("Package/Filesystem");
			if(filesystemNode != NULL)
			{
				rootDir = PsfFs::ParseDirectoryNode(filesystemNode);
			}
		}
	}

	if(!rootDir)
	{
		printf("No filesystem found. Aborting.\r\n");
		return -1;
	}

	//Derive base path
	boost::filesystem::path basePath(argv[1]);
	basePath = boost::filesystem::complete(basePath);
	basePath = basePath.remove_leaf();

	try
	{
		Framework::CStdStream output(argv[2], "wb");

		//Write header
		PSFHEADER header;
		header.signature[0] = 'P';
		header.signature[1] = 'S';
		header.signature[2] = 'F';
		header.version		= 0x03;
		header.reservedSize	= 0;
		header.programSize	= 0;
		header.programCrc	= 0;
		output.Write(&header, sizeof(PSFHEADER));

		//Write reserved
		uint64 reservedSize = output.Tell();
		{
			PsfFs::CWriter::Write(output, rootDir, basePath);
		}
		reservedSize = output.Tell() - reservedSize;

		//Writeback header
		header.reservedSize = static_cast<uint32>(reservedSize);

		output.Seek(0, Framework::STREAM_SEEK_SET);
		output.Write(&header, sizeof(PSFHEADER));
	}
	catch(const std::exception& exception)
	{
		printf("Error: %s\r\n", exception.what());
	}

	return 0;
}
