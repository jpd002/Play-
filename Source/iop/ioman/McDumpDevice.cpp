#include "McDumpDevice.h"
#include <cassert>
#include <cstring>
#include "maybe_unused.h"
#include "MemStream.h"

using namespace Iop;
using namespace Iop::Ioman;

CMcDumpDevice::CMcDumpDevice(DumpContent content)
    : m_content(std::move(content))
    , m_contentReader(m_content.data(), m_content.size())
    , m_dumpReader(m_contentReader)
{
}

Framework::CStream* CMcDumpDevice::GetFile(uint32 flags, const char* path)
{
	assert(flags == OPEN_FLAG_RDONLY);

	//We don't support subdirs here
	if(*path == '/') path++;
	FRAMEWORK_MAYBE_UNUSED auto next = strchr(path, '/');
	assert(!next);

	auto directory = m_dumpReader.ReadRootDirectory();
	for(const auto& dirEntry : directory)
	{
		if(dirEntry.mode & CMcDumpReader::DF_DIRECTORY) continue;
		if(!strcmp(dirEntry.name, path))
		{
			//Got a match
			auto fileContents = m_dumpReader.ReadFile(dirEntry.cluster, dirEntry.length);
			assert(fileContents.size() == dirEntry.length);
			auto stream = new Framework::CMemStream();
			stream->Write(fileContents.data(), fileContents.size());
			stream->Seek(0, Framework::STREAM_SEEK_SET);
			return stream;
		}
	}

	return nullptr;
}

DirectoryIteratorPtr CMcDumpDevice::GetDirectory(const char*)
{
	return DirectoryIteratorPtr();
}
