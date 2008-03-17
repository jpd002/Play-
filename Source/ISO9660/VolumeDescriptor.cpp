#include <string.h>
#include <stdexcept>
#include "VolumeDescriptor.h"

using namespace Framework;
using namespace ISO9660;
using namespace std;

CVolumeDescriptor::CVolumeDescriptor(CStream* stream)
{
    //Starts at LBA 16
    stream->Seek(0x8000, STREAM_SEEK_SET);
    stream->Read(&m_nType, 1);

    if(m_nType != 0x01)
    {
        throw runtime_error("Invalid ISO9660 Volume Descriptor.");
    }

    stream->Read(m_sStdId, 5);
    m_sStdId[5] = 0x00;

    if(strcmp(m_sStdId, "CD001"))
    {
        throw runtime_error("Invalid ISO9660 Volume Descriptor.");
    }

    stream->Seek(34, STREAM_SEEK_CUR);

    stream->Read(m_sVolumeId, 32);
    m_sVolumeId[32] = 0x00;

    stream->Seek(68, STREAM_SEEK_CUR);

    stream->Read(&m_nLPathTableAddress, 4);
    stream->Read(&m_nMPathTableAddress, 4);
}

CVolumeDescriptor::~CVolumeDescriptor()
{

}

uint32 CVolumeDescriptor::GetLPathTableAddress() const
{
    return m_nLPathTableAddress;
}

uint32 CVolumeDescriptor::GetMPathTableAddress() const
{
    return m_nMPathTableAddress;
}
