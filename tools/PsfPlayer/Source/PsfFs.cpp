#include "PsfFs.h"
#include "StdStream.h"
#include "OffsetStream.h"
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <zlib.h>

using namespace Framework;
using namespace std;
using namespace boost;

#define HEADERSIZE      0x10

CPsfFs::CPsfFs(const char* loadPath)
{
    CStdStream stream(loadPath, "rb");
    char signature[4];
    stream.Read(signature, 3);
    signature[3] = 0;
    if(strcmp(signature, "PSF"))
    {
        throw runtime_error("Invalid PSF file (Invalid signature).");
    }
    uint8 psfType = stream.Read8();
    if(psfType != 0x02)
    {
        throw runtime_error("PSF type isn't 'PlayStation2'.");
    }
    uint32 reservedSize = stream.Read32();
    uint32 programSize = stream.Read32();
    uint32 programCrc = stream.Read32();
    if(programSize != 0 || programCrc != 0)
    {
        throw runtime_error("Invalid PS2 type PSF file (Has a non zero program section).");
    }

    //Read the tag
    stream.Seek(0, STREAM_SEEK_END);
    uint64 tagBeginPosition = HEADERSIZE + programSize + reservedSize;
    uint64 tagSize = stream.Tell() - tagBeginPosition;
    stream.Seek(tagBeginPosition, STREAM_SEEK_SET);

    {
        char* tagBuffer = reinterpret_cast<char*>(_alloca(tagSize + 1));
        stream.Read(tagBuffer, tagSize);
        tagBuffer[tagSize] = 0;
        m_tag = tagBuffer;
    }

    //Need to check for libs
    
    COffsetStream reservedDataStream(stream, HEADERSIZE);
    reservedDataStream.Seek(0, STREAM_SEEK_SET);
    ReadDirectory(reservedDataStream, m_root);
}

CPsfFs::~CPsfFs()
{

}

const CPsfFs::FILE* CPsfFs::GetFile(const char* path) const
{
    string transformPath(path);
    string::size_type position = transformPath.find('\\');
    while(position != string::npos)
    {
        transformPath[position] = '/';
        position = transformPath.find('\\');
    }
    return GetFileDetail(m_root, transformPath.c_str());
}

string CPsfFs::GetPsfLibAttributeName(unsigned int libIndex)
{
    if(libIndex == 0) 
    {
        return string("_lib");
    }
    else
    {
        return string("_lib") + lexical_cast<string>(libIndex + 1);
    }
}

void CPsfFs::ReadFile(CStream& stream, FILE& file)
{
    unsigned int sizeTableEntryCount = (file.size + file.blockSize - 1) / file.blockSize;
    uint32* sizeTable = reinterpret_cast<uint32*>(_alloca(sizeTableEntryCount * sizeof(uint32)));
    for(unsigned int entry = 0; entry < sizeTableEntryCount; entry++)
    {
        sizeTable[entry] = stream.Read32();
        if(sizeTable[entry] > file.blockSize)
        {
            throw runtime_error("Invalid block size.");
        }
    }
    uint8* block = reinterpret_cast<uint8*>(_alloca(file.blockSize));
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

void CPsfFs::ReadDirectory(CStream& stream, DIRECTORY& baseDirectory)
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
        stream.Seek(node.offset, STREAM_SEEK_SET);
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
        stream.Seek(currentPosition, STREAM_SEEK_SET);
    }
}

const CPsfFs::FILE* CPsfFs::GetFileDetail(const DIRECTORY& directory, const char* path) const
{
    if(*path == '/') path++;
    const char* separator = strchr(path, '/');

    if(separator == NULL)
    {
        for(DIRECTORY::FileListType::const_iterator nodeIterator(directory.fileList.begin());
            nodeIterator != directory.fileList.end(); nodeIterator++)
        {
            const NODE* node(*nodeIterator);
            if(!strcmp(node->name, path))
            {
                return dynamic_cast<const FILE*>(node);
            }
        }
    }
    else
    {
        //Gotta look in subdir
    }
}
