#include "ChdStreamSupport.h"
#include <cassert>
#include <libchdr/chd.h>
#include "Stream.h"

static size_t stream_core_fread(void* buffer, size_t elemSize, size_t elemCount, core_file* file)
{
	assert(elemSize == 1);
	auto stream = reinterpret_cast<Framework::CStream*>(file->argp);
	return stream->Read(buffer, elemSize * elemCount);
}

static int stream_core_fseek(core_file* file, INT64 position, int whence)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file->argp);
	stream->Seek(position, static_cast<Framework::STREAM_SEEK_DIRECTION>(whence));
	return 0;
}

static UINT64 stream_core_fsize(core_file* file)
{
	auto stream = reinterpret_cast<Framework::CStream*>(file->argp);
	auto currPos = stream->Tell();
	auto size = stream->GetLength();
	stream->Seek(currPos, Framework::STREAM_SEEK_SET);
	return size;
}

static int stream_core_fclose(core_file* file)
{
	delete file;
	return 0;
}

core_file* ChdStreamSupport::CreateFileFromStream(Framework::CStream* stream)
{
	auto file = new core_file;
	file->argp = stream;
	file->fread = &stream_core_fread;
	file->fseek = &stream_core_fseek;
	file->fsize = &stream_core_fsize;
	file->fclose = &stream_core_fclose;
	return file;
}
