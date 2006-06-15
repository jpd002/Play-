#include <string.h>
#include "VolumeDescriptor.h"

using namespace Framework;
using namespace ISO9660;

CVolumeDescriptor::CVolumeDescriptor(CStream* pStream)
{
	//Starts at LBA 16
	pStream->Seek(0x8000, STREAM_SEEK_SET);
	pStream->Read(&m_nType, 1);

	if(m_nType != 0x01)
	{
		throw "Invalid ISO9660 Volume Descriptor.";
	}

	pStream->Read(m_sStdId, 5);
	m_sStdId[5] = 0x00;

	if(strcmp(m_sStdId, "CD001"))
	{
		throw "Invalid ISO9660 Volume Descriptor.";
	}

	pStream->Seek(34, STREAM_SEEK_CUR);

	pStream->Read(m_sVolumeId, 32);
	m_sVolumeId[32] = 0x00;

	pStream->Seek(68, STREAM_SEEK_CUR);

	pStream->Read(&m_nLPathTableAddress, 4);
	pStream->Read(&m_nMPathTableAddress, 4);
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
