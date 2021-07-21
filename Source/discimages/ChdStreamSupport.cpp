#include "ChdStreamSupport.h"
#include <libchdr/chd.h>
#include "Stream.h"

static UINT64 stream_core_fread(void* file, void* buffer, UINT64 size)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file);
	return stream->Read(buffer, size);
}

static void stream_core_fseek(void* file, INT64 position, int whence)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file);
	stream->Seek(position, static_cast<Framework::STREAM_SEEK_DIRECTION>(whence));
}

static UINT64 stream_core_ftell(void* file)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file);
	return stream->Tell();
}

core_file* ChdStreamSupport::CreateFileFromStream(Framework::CStream* stream)
{
	auto result = core_falloc();
	result->user_data = stream;
	result->read_function = &stream_core_fread;
	result->tell_function = &stream_core_ftell;
	result->seek_function = &stream_core_fseek;
	return result;
}
