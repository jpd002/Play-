#include <zlib.h>
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include "PsfFs.h"
#include "PtrStream.h"
#include "stricmp.h"

CPsfFs::CPsfFs()
{

}

CPsfFs::~CPsfFs()
{

}

void CPsfFs::AppendArchive(const CPsfBase& archive)
{
	Framework::CPtrStream fsStream(archive.GetReserved(), archive.GetReservedSize());
	ReadDirectory(fsStream, m_root);
}

void CPsfFs::ReadFile(Framework::CStream& stream, FILE& file)
{
	//Read size table
	unsigned int sizeTableEntryCount = (file.size + file.blockSize - 1) / file.blockSize;
	uint32* sizeTable = reinterpret_cast<uint32*>(alloca(sizeTableEntryCount * sizeof(uint32)));
	stream.Read(sizeTable, sizeof(uint32) * sizeTableEntryCount);

	//Allocate memory to read compressed blocks
	uint32 maxSize = *std::max_element(sizeTable, sizeTable + sizeTableEntryCount);
	uint8* block = reinterpret_cast<uint8*>(alloca(maxSize));

	//Read away
	uint32 position = 0;
	for(unsigned int entry = 0; entry < sizeTableEntryCount; entry++)
	{
		uint32 blockSize = sizeTable[entry];
		stream.Read(block, blockSize);
		uLongf bufferSize = file.size - position;
		uncompress(file.data + position, &bufferSize, block, blockSize);
		assert(bufferSize <= file.blockSize);
		position += bufferSize;
	}
}

void CPsfFs::ReadDirectory(Framework::CStream& stream, DIRECTORY& baseDirectory)
{
	uint32 entryCount = stream.Read32();
	for(uint32 entry = 0; entry < entryCount; entry++)
	{
		NODE node;
		stream.Read(node.name, 36);
		node.name[36] = 0;
		node.offset = stream.Read32();
		node.size = stream.Read32();
		node.blockSize = stream.Read32();

		uint64 currentPosition = stream.Tell();
		stream.Seek(node.offset, Framework::STREAM_SEEK_SET);
		if(node.IsDirectory())
		{
			//Check if this already exists.
			NODE* existingNode = GetFileFindNode(baseDirectory, node.name);
			if(existingNode != NULL)
			{
				DIRECTORY* existingDir = dynamic_cast<DIRECTORY*>(existingNode);
				assert(existingDir != NULL);
				if(existingDir != NULL)
				{
					ReadDirectory(stream, *existingDir);
				}
			}
			else
			{
				DIRECTORY* directory = new DIRECTORY(node);
				ReadDirectory(stream, *directory);
				baseDirectory.fileList.push_back(directory);
			}
		}
		else
		{
			FILE* file = new FILE(node);
			ReadFile(stream, *file);
			baseDirectory.fileList.push_back(file);
		}
		stream.Seek(currentPosition, Framework::STREAM_SEEK_SET);
	}
}

Framework::CStream* CPsfFs::GetFile(const char* path)
{
	std::string transformPath(path);
	std::string::size_type position = transformPath.find('\\');
	while(position != std::string::npos)
	{
		transformPath[position] = '/';
		position = transformPath.find('\\');
	}
	const FILE* file = GetFileDetail(m_root, transformPath.c_str());
	if(!file)
	{
		return nullptr;
	}
	return new Framework::CPtrStream(file->data, file->size);
}

CPsfFs::NODE* CPsfFs::GetFileFindNode(const DIRECTORY& directory, const char* path)
{
	for(auto nodeIterator = std::begin(directory.fileList);
		nodeIterator != std::end(directory.fileList); nodeIterator++)
	{
		NODE* node(*nodeIterator);
		if(!stricmp(node->name, path))
		{
			return node;
		}
	}

	return nullptr;
}

const CPsfFs::FILE* CPsfFs::GetFileDetail(const DIRECTORY& directory, const char* path) const
{
	if(*path == '/') path++;
	const char* separator = strchr(path, '/');

	std::string filename(path, separator ? separator : path + strlen(path));

	const NODE* node = GetFileFindNode(directory, filename.c_str());
	if(!node)
	{
		return nullptr;
	}

	if(!separator)
	{
		return dynamic_cast<const FILE*>(node);
	}
	else
	{
		const DIRECTORY* subDir = dynamic_cast<const DIRECTORY*>(node);
		if(!subDir)
		{
			return nullptr;
		}
		return GetFileDetail(*subDir, separator + 1);
	}
}
