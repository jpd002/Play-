#include "PsfFsWriter.h"
#include <boost/filesystem/operations.hpp>
#include <boost/shared_array.hpp>
#include <zlib.h>
#include <vector>

using namespace PsfFs;

CWriter::CWriter(const boost::filesystem::path& basePath, uint64 baseOffset)
: m_basePath(basePath)
, m_baseOffset(baseOffset)
{

}

CWriter::~CWriter()
{

}

void CWriter::Write(Framework::CStdStream& output, const DirectoryPtr& directory, const boost::filesystem::path& basePath)
{
	CWriter writer(basePath, output.Tell());
	writer.WriteDirectory(output, directory, NULL);
}

void CWriter::WriteFile(Framework::CStdStream& output, const FilePtr& file, DIRENTRY* dirEntry)
{
	assert(dirEntry != NULL);

	try
	{
		boost::filesystem::path filePath(m_basePath / file->source);

		Framework::CStdStream input(filePath.string().c_str(), "rb");

		uint32 offset = static_cast<uint32>(output.Tell());
		uint32 fileSize = static_cast<uint32>(input.GetLength());

		const unsigned int blockSize = 0x1000;
		unsigned int blockCount = (fileSize + blockSize - 1) / blockSize;

		dirEntry->offset			= offset - m_baseOffset;
		dirEntry->blockSize			= blockSize;
		dirEntry->uncompressedSize	= fileSize;

		boost::shared_array<uint32> blockSizeTable(new uint32[blockCount]);
		memset(blockSizeTable.get(), 0, blockCount * sizeof(uint32));

		//Write block size table
		output.Write(blockSizeTable.get(), blockCount * sizeof(uint32));

		uint32 outputBufferSize = compressBound(blockSize);
		boost::shared_array<uint8> inputBuffer(new uint8[blockSize]);
		boost::shared_array<uint8> outputBuffer(new uint8[outputBufferSize]);
		int blockIndex = 0;
		while(!input.IsEOF())
		{
			uint32 resultBufferSize = outputBufferSize;
			input.Read(inputBuffer.get(), blockSize);
			compress(outputBuffer.get(), &resultBufferSize, inputBuffer.get(), blockSize);
			output.Write(outputBuffer.get(), resultBufferSize);
			blockSizeTable[blockIndex++] = resultBufferSize; 
		}
		assert(blockIndex == blockCount);

		//Update block size table
		output.Seek(offset, Framework::STREAM_SEEK_SET);
		output.Write(blockSizeTable.get(), blockCount * sizeof(uint32));
		output.Seek(0, Framework::STREAM_SEEK_END);
	}
	catch(const std::exception& exception)
	{
		throw std::runtime_error("Cannot process file '" + file->source + "': " + std::string(exception.what()));
	}
}

void CWriter::WriteDirectory(Framework::CStdStream& output, const DirectoryPtr& directory, DIRENTRY* dirEntry)
{
	std::vector<DIRENTRY> dirEntries;
	dirEntries.reserve(directory->nodes.size());

	uint32 dirOffset = static_cast<uint32>(output.Tell());

	if(dirEntry != NULL)
	{
		dirEntry->blockSize			= 0;
		dirEntry->uncompressedSize	= 0;
		dirEntry->offset			= dirOffset - m_baseOffset;
	}

	//Write entry count
	output.Write32(directory->nodes.size());
	dirOffset += 4;

	//First, compute the size of the folder info
	for(NodeList::const_iterator nodeIterator(directory->nodes.begin());
		nodeIterator != directory->nodes.end(); nodeIterator++)
	{
		const NodePtr& node(*nodeIterator);
		DIRENTRY entry;
		if(strlen(node->name.c_str()) > 36)
		{
			printf("Warning: Entry name '%s' exceeds 36 characters and will be truncated.\r\n", node->name.c_str());
		}
		strncpy(reinterpret_cast<char*>(entry.name), node->name.c_str(), 36);
		entry.offset			= 0;
		entry.blockSize			= 0;
		entry.uncompressedSize	= 0;
		dirEntries.push_back(entry);
	}

	//Write directory info
	output.Write(&dirEntries[0], (sizeof(DIRENTRY) * directory->nodes.size()));

	//Write all files
	{
		int i = 0;
		for(NodeList::const_iterator nodeIterator(directory->nodes.begin());
			nodeIterator != directory->nodes.end(); nodeIterator++, i++)
		{
			const NodePtr& node(*nodeIterator);
			DIRENTRY& entry = dirEntries[i];
			const FilePtr file = std::tr1::dynamic_pointer_cast<FILE>(node);
			if(file)
			{
				WriteFile(output, file, &entry);
			}
		}
	}

	//Write directories
	{
		int i = 0;
		for(NodeList::const_iterator nodeIterator(directory->nodes.begin());
			nodeIterator != directory->nodes.end(); nodeIterator++, i++)
		{
			const NodePtr& node(*nodeIterator);
			DIRENTRY& entry = dirEntries[i];
			const DirectoryPtr directory = std::tr1::dynamic_pointer_cast<DIRECTORY>(node);
			if(directory)
			{
				WriteDirectory(output, directory, &entry);
			}
		}
	}

	//Update directory
	output.Seek(dirOffset, Framework::STREAM_SEEK_SET);
	output.Write(&dirEntries[0], (sizeof(DIRENTRY) * directory->nodes.size()));
	output.Seek(0, Framework::STREAM_SEEK_END);
}
