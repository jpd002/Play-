#include "HardDiskDumpDevice.h"
#include "HardDiskDevice.h"
#include "hdd/ApaReader.h"
#include "StringUtils.h"
#include "MemStream.h"

using namespace Iop;
using namespace Iop::Ioman;

CHardDiskDumpDevice::CHardDiskDumpDevice(std::unique_ptr<Framework::CStream> stream)
	: m_stream(std::move(stream))
{
	
}

Framework::CStream* CHardDiskDumpDevice::GetFile(uint32 flags, const char* path)
{
	assert(flags == OPEN_FLAG_RDONLY);
	Hdd::CApaReader reader(*m_stream);
	uint32 partitionLba = reader.FindPartition(path);
	if(partitionLba == -1)
	{
		return nullptr;
	}
	return new CHardDiskPartition();
}

DirectoryIteratorPtr CHardDiskDumpDevice::GetDirectory(const char*)
{
	return DirectoryIteratorPtr();
}

DevicePtr CHardDiskDumpDevice::Mount(const char* path)
{
	auto mountParams = StringUtils::Split(path, ',', true);
	Hdd::CApaReader reader(*m_stream);
	uint32 partitionLba = reader.FindPartition(mountParams[0].c_str());
	if(partitionLba == -1)
	{
		assert(false);
		return DevicePtr();
	}
	return std::make_shared<CHardDiskDumpPartitionDevice>(*m_stream, partitionLba);
}

CHardDiskDumpPartitionDevice::CHardDiskDumpPartitionDevice(Framework::CStream& stream, uint32 partitionLba)
	: m_pfsReader(stream, partitionLba)
{
	
}

Framework::CStream* CHardDiskDumpPartitionDevice::GetFile(uint32 flags, const char* path)
{
	assert((flags & OPEN_FLAG_ACCMODE) == OPEN_FLAG_RDONLY);
	return m_pfsReader.GetFileStream(path);
}

DirectoryIteratorPtr CHardDiskDumpPartitionDevice::GetDirectory(const char*)
{
	assert(false);
	return DirectoryIteratorPtr();
}
