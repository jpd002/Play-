#include "HardDiskDumpDevice.h"
#include <cassert>
#include <cstring>
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

DirectoryIteratorPtr CHardDiskDumpDevice::GetDirectory(const char* path)
{
	assert(strlen(path) == 0);
	Hdd::CApaReader reader(*m_stream);
	auto partitions = reader.GetPartitions();
	return std::make_unique<CHardDiskDumpDirectoryIterator>(std::move(partitions));
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

bool CHardDiskDumpDevice::TryGetStat(const char* path, bool& succeeded, STAT& stat)
{
	auto mountParams = StringUtils::Split(path, ',', true);
	Hdd::APA_HEADER partitionHeader = {};
	Hdd::CApaReader reader(*m_stream);
	if(!reader.TryFindPartition(mountParams[0].c_str(), partitionHeader))
	{
		succeeded = false;
		return true;
	}
	succeeded = true;
	stat = {};
	stat.mode = partitionHeader.type;
	stat.attr = partitionHeader.flags;
	stat.loSize = partitionHeader.length;
	return true;
}

CHardDiskDumpDirectoryIterator::CHardDiskDumpDirectoryIterator(std::vector<Hdd::APA_HEADER> partitions)
    : m_partitions(std::move(partitions))
{
}

void CHardDiskDumpDirectoryIterator::ReadEntry(DIRENTRY* entry)
{
	*entry = {};
	while(!IsDone())
	{
		const auto& partition = m_partitions[m_index++];
		if(strlen(partition.id) == 0) continue;
		static_assert(sizeof(entry->name) >= sizeof(partition.id));
		strncpy(entry->name, partition.id, Hdd::APA_HEADER::ID_SIZE);
		entry->name[Hdd::APA_HEADER::ID_SIZE - 1] = 0;
		break;
	}
}

bool CHardDiskDumpDirectoryIterator::IsDone()
{
	return (m_index == m_partitions.size());
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
