#include <zlib.h>
#include <stdexcept>
#include "PsfDevice.h"
#include "PtrStream.h"
#include "stricmp.h"

using namespace PS2;
using namespace Framework;
using namespace std;

CPsfDevice::CPsfDevice()
{

}

CPsfDevice::~CPsfDevice()
{

}

void CPsfDevice::AppendArchive(const CPsfBase& archive)
{
    CPtrStream fsStream(archive.GetReserved(), archive.GetReservedSize());
	ReadDirectory(fsStream, m_root);
}

void CPsfDevice::ReadFile(CStream& stream, FILE& file)
{
    unsigned int sizeTableEntryCount = (file.size + file.blockSize - 1) / file.blockSize;
    uint32* sizeTable = reinterpret_cast<uint32*>(alloca(sizeTableEntryCount * sizeof(uint32)));
    for(unsigned int entry = 0; entry < sizeTableEntryCount; entry++)
    {
        sizeTable[entry] = stream.Read32();
        if(sizeTable[entry] > file.blockSize)
        {
            throw runtime_error("Invalid block size.");
        }
    }
    uint8* block = reinterpret_cast<uint8*>(alloca(file.blockSize));
    uint32 position = 0;
    for(unsigned int entry = 0; entry < sizeTableEntryCount; entry++)
    {
        uint32 blockSize = sizeTable[entry];
        stream.Read(block, blockSize);
        uint32 bufferSize = file.size - position;
        uncompress(file.data + position, &bufferSize, block, blockSize);
        position += bufferSize;
    }
}

void CPsfDevice::ReadDirectory(CStream& stream, DIRECTORY& baseDirectory)
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
            DIRECTORY* directory = new DIRECTORY(node);
            ReadDirectory(stream, *directory);
            baseDirectory.fileList.push_back(directory);
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

CStream* CPsfDevice::GetFile(uint32 mode, const char* path)
{
    if(mode != CDevice::O_RDONLY)
    {
        printf("%s: Attempting to open a file in non read mode.\r\n", __FUNCTION__);
        return NULL;
    }
    string transformPath(path);
    string::size_type position = transformPath.find('\\');
    while(position != string::npos)
    {
        transformPath[position] = '/';
        position = transformPath.find('\\');
    }
    const FILE* file = GetFileDetail(m_root, transformPath.c_str());
    if(file == NULL)
    {
        return NULL;
    }
    return new CPtrStream(file->data, file->size);
}

const CPsfDevice::NODE* CPsfDevice::GetFileFindNode(const DIRECTORY& directory, const char* path) const
{
    for(DIRECTORY::FileListType::const_iterator nodeIterator(directory.fileList.begin());
        nodeIterator != directory.fileList.end(); nodeIterator++)
    {
        const NODE* node(*nodeIterator);
        if(!stricmp(node->name, path))
        {
            return node;
        }
    }

    return NULL;
}

const CPsfDevice::FILE* CPsfDevice::GetFileDetail(const DIRECTORY& directory, const char* path) const
{
    if(*path == '/') path++;
    const char* separator = strchr(path, '/');

    string filename(path, separator != NULL ? separator : path + strlen(path));

    const NODE* node = GetFileFindNode(directory, filename.c_str());
    if(node == NULL)
    {
        return NULL;
    }

    if(separator == NULL)
    {
        return dynamic_cast<const FILE*>(node);
    }
    else
    {
        const DIRECTORY* subDir = dynamic_cast<const DIRECTORY*>(node);
        if(subDir == NULL)
        {
            return NULL;
        }
        return GetFileDetail(*subDir, separator + 1);
    }
}
