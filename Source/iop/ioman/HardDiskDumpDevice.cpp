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
	Hdd::APA_HEADER partitionHeader = {};
	Hdd::CApaReader reader(*m_stream);
	if(!reader.TryFindPartition(path, partitionHeader))
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
	Hdd::APA_HEADER partitionHeader = {};
	Hdd::CApaReader reader(*m_stream);
	if(!reader.TryFindPartition(mountParams[0].c_str(), partitionHeader))
	{
		assert(false);
		return DevicePtr();
	}
	return std::make_shared<CHardDiskDumpPartitionDevice>(*m_stream, partitionHeader);
}

CHardDiskDumpPartitionDevice::CHardDiskDumpPartitionDevice(Framework::CStream& stream, const Hdd::APA_HEADER& partitionHeader)
	: m_pfsReader(stream, partitionHeader)
{
	
}

Framework::CStream* CHardDiskDumpPartitionDevice::GetFile(uint32 accessType, const char* path)
{
	accessType &= ~OPEN_FLAG_NOWAIT;
	assert(accessType == OPEN_FLAG_RDONLY);
	std::string absPath = path;
	assert(!absPath.empty());
	if(absPath[0] != '/')
	{
		//TODO: Append current directory
		absPath = '/' + absPath;
	}
	return m_pfsReader.GetFileStream(absPath.c_str());
}

DirectoryIteratorPtr CHardDiskDumpPartitionDevice::GetDirectory(const char*)
{
	assert(false);
	return DirectoryIteratorPtr();
}
