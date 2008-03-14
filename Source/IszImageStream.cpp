#include "IszImageStream.h"
#include <stdexcept>
#include <assert.h>
#include "bzlib.h"
#include "zlib.h"
#include "StdStream.h"

using namespace Framework;
using namespace std;

CIszImageStream::CIszImageStream(CStream* baseStream) :
m_baseStream(baseStream),
m_blockDescriptorTable(NULL),
m_cachedBlock(NULL),
m_readBuffer(NULL),
m_cachedBlockNumber(-1),
m_position(0)
{
    if(baseStream == NULL)
    {
        throw runtime_error("Null base stream supplied.");
    }

    baseStream->Read(&m_header, sizeof(HEADER));

    assert(m_header.hasPassword == 0);
    assert(m_header.segmentPtrOffset == 0);

    if(m_header.blockPtrOffset == 0)
    {
        throw runtime_error("Block Descriptor Table not present.");
    }
    if(m_header.blockPtrLength != 3)
    {
        throw runtime_error("Unsupported block descriptor size.");
    }

    ReadBlockDescriptorTable();
    m_cachedBlock = new uint8[m_header.blockSize];
    m_readBuffer = new uint8[m_header.blockSize];
}

CIszImageStream::~CIszImageStream()
{
    delete [] m_cachedBlock;
    delete [] m_readBuffer;
    delete [] m_blockDescriptorTable;
    delete m_baseStream;
}

void CIszImageStream::Seek(int64 position, STREAM_SEEK_DIRECTION origin)
{
    switch(origin)
    {
    case Framework::STREAM_SEEK_CUR:
        m_position += position;
        break;
    case Framework::STREAM_SEEK_SET:
        m_position = position;
        break;
    case Framework::STREAM_SEEK_END:
        m_position = GetTotalSize();
        break;
    }
}

uint64 CIszImageStream::Tell()
{
    return m_position;
}

uint64 CIszImageStream::Read(void* buffer, uint64 size)
{
    uint64 bytesRead = 0;
    uint8* inputBuffer = reinterpret_cast<uint8*>(buffer);
    while(size != 0)
    {
        if(IsEOF())
        {
            break;
        }
        SyncCache();
        uint64 blockPosition = (m_position % m_header.blockSize);
        uint64 sizeLeft = m_header.blockSize - blockPosition;
        uint64 sizeToRead = min<uint64>(size, sizeLeft);
        memcpy(inputBuffer, m_cachedBlock + blockPosition, static_cast<size_t>(sizeToRead));
        m_position += sizeToRead;
        size -= sizeToRead;
        inputBuffer += sizeToRead;
        bytesRead += sizeToRead;
    }
    return bytesRead;
}

uint64 CIszImageStream::Write(const void* buffer, uint64 size)
{
    throw exception();
}

bool CIszImageStream::IsEOF()
{
    return (m_position >= GetTotalSize());
}

void CIszImageStream::ReadBlockDescriptorTable()
{
    const char* key = "IsZ!";
    unsigned int cryptedTableLength = m_header.blockNumber * m_header.blockPtrLength;
    uint8* cryptedTable = new uint8[cryptedTableLength];
    m_baseStream->Seek(m_header.blockPtrOffset, Framework::STREAM_SEEK_SET);
    m_baseStream->Read(cryptedTable, cryptedTableLength);
    for(unsigned int i = 0; i < cryptedTableLength; i++)
    {
        cryptedTable[i] ^= ~key[i & 3];
    }

    m_blockDescriptorTable = new BLOCKDESCRIPTOR[m_header.blockNumber];
    for(unsigned int i = 0; i < m_header.blockNumber; i++)
    {
        uint32 value = *reinterpret_cast<uint32*>(&cryptedTable[i * m_header.blockPtrLength]);
        value &= 0xFFFFFF;
        m_blockDescriptorTable[i].size = value & 0x3FFFFF;
        m_blockDescriptorTable[i].storageType = static_cast<uint8>(value >> 22);
    }

    delete [] cryptedTable;
}

uint64 CIszImageStream::GetTotalSize() const
{
    return static_cast<uint64>(m_header.totalSectors) * static_cast<uint64>(m_header.sectorSize);
}

const CIszImageStream::BLOCKDESCRIPTOR& CIszImageStream::SeekToBlock(uint64 blockNumber)
{
    assert(blockNumber < m_header.blockNumber);
    uint64 seekPosition = m_header.dataOffset;
    for(unsigned int i = 0; i < blockNumber; i++)
    {
        const BLOCKDESCRIPTOR& blockDescriptor = m_blockDescriptorTable[i];
        if(blockDescriptor.storageType != ADI_ZERO)
        {
            seekPosition += blockDescriptor.size;
        }
    }
    m_baseStream->Seek(seekPosition, Framework::STREAM_SEEK_SET);
    return m_blockDescriptorTable[blockNumber];
}

void CIszImageStream::SyncCache()
{
    uint64 currentSector = (m_position / m_header.sectorSize);
    uint64 neededBlock = (currentSector * m_header.sectorSize) / m_header.blockSize;
    if(neededBlock == m_cachedBlockNumber)
    {
        return;
    }
    if(neededBlock >= m_header.blockNumber)
    {
        throw runtime_error("Trying to read past eof.");
    }

    const BLOCKDESCRIPTOR& blockDescriptor = SeekToBlock(neededBlock);
    memset(m_cachedBlock, 0, m_header.blockSize);
    switch(blockDescriptor.storageType)
    {
    case ADI_ZERO:
        ReadZeroBlock(blockDescriptor.size);
        break;
    case ADI_ZLIB:
        ReadGzipBlock(blockDescriptor.size);
        break;
    case ADI_BZ2:
        ReadBz2Block(blockDescriptor.size);
        break;
    default:
        throw runtime_error("Unsupported block storage mode.");
        break;
    }
    m_cachedBlockNumber = neededBlock;
}

void CIszImageStream::ReadZeroBlock(uint32 compressedBlockSize)
{
    if(compressedBlockSize != m_header.blockSize)
    {
        throw runtime_error("Invalid zero block.");
    }
}

void CIszImageStream::ReadGzipBlock(uint32 compressedBlockSize)
{
    m_baseStream->Read(m_readBuffer, compressedBlockSize);
    uLongf destLength = m_header.blockSize;
    if(uncompress(
        reinterpret_cast<Bytef*>(m_cachedBlock), &destLength,
        reinterpret_cast<Bytef*>(m_readBuffer), compressedBlockSize) != Z_OK)
    {
        throw runtime_error("Error decompressing zlib block.");
    }
}

void CIszImageStream::ReadBz2Block(uint32 compressedBlockSize)
{
    m_baseStream->Read(m_readBuffer, compressedBlockSize);
    //Force BZ2 header
    m_readBuffer[0] = 'B';
    m_readBuffer[1] = 'Z';
    m_readBuffer[2] = 'h';
    unsigned int destLength = m_header.blockSize;
    if(BZ2_bzBuffToBuffDecompress(
        reinterpret_cast<char*>(m_cachedBlock), &destLength, 
        reinterpret_cast<char*>(m_readBuffer), compressedBlockSize, 0, 0) != BZ_OK)
    {
        throw runtime_error("Error decompressing bz2 block.");
    }
}
