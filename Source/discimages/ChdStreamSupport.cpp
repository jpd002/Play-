#include <cassert>
#include "libchdr/chd.h"
#include "Stream.h"

core_file* core_fopen(const char*)
{
	assert(false);
	return nullptr;
}

void core_fclose(core_file*)
{
	assert(false);
}

UINT64 core_fread(core_file* file, void* buffer, UINT64 size)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file);
	return stream->Read(buffer, size);
}

void core_fseek(core_file* file, INT64 position, int whence)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file);
	stream->Seek(position, static_cast<Framework::STREAM_SEEK_DIRECTION>(whence));
}

UINT64 core_ftell(core_file* file)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file);
	return stream->Tell();
}
