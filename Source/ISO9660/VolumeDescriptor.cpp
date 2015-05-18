#include <string.h>
#include <stdexcept>
#include "VolumeDescriptor.h"

using namespace ISO9660;

CVolumeDescriptor::CVolumeDescriptor(Framework::CStream& stream)
{
	stream.Read(&m_type, 1);

	if(m_type != 0x01)
	{
		throw std::runtime_error("Invalid ISO9660 Volume Descriptor.");
	}

	stream.Read(m_stdId, 5);
	m_stdId[5] = 0x00;

	if(strcmp(m_stdId, "CD001"))
	{
		throw std::runtime_error("Invalid ISO9660 Volume Descriptor.");
	}

	stream.Seek(34, Framework::STREAM_SEEK_CUR);

	stream.Read(m_volumeId, 32);
	m_volumeId[32] = 0x00;

	stream.Seek(68, Framework::STREAM_SEEK_CUR);

	stream.Read(&m_LPathTableAddress, 4);
	stream.Read(&m_MPathTableAddress, 4);
}

CVolumeDescriptor::~CVolumeDescriptor()
{

}

uint32 CVolumeDescriptor::GetLPathTableAddress() const
{
	return m_LPathTableAddress;
}

uint32 CVolumeDescriptor::GetMPathTableAddress() const
{
	return m_MPathTableAddress;
}
