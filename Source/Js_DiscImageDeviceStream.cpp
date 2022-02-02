#include "Js_DiscImageDeviceStream.h"
#include <stdexcept>
#include <cassert>
#include <emscripten.h>
#include <unistd.h>

void CJsDiscImageDeviceStream::Seek(int64 position, Framework::STREAM_SEEK_DIRECTION whence)
{
	switch(whence)
	{
	case Framework::STREAM_SEEK_SET:
		m_position = position;
		break;
	case Framework::STREAM_SEEK_CUR:
		m_position += position;
		break;
	case Framework::STREAM_SEEK_END:
		m_position = MAIN_THREAD_EM_ASM_INT({return Module.discImageDevice.getFileSize()});
		break;
	}
}

uint64 CJsDiscImageDeviceStream::Tell()
{
	return m_position;
}

uint64 CJsDiscImageDeviceStream::Read(void* buffer, uint64 size)
{
	if(size == 0) return 0;

	assert(size <= std::numeric_limits<uint32>::max());

	uint32 positionLow = static_cast<uint32>(m_position);
	uint32 positionHigh = static_cast<uint32>(m_position >> 32);

	MAIN_THREAD_EM_ASM({
		let posLow = $1 >>> 0;
		let posHigh = $2 >>> 0;
		let position = posLow + (posHigh * 4294967296);
		Module.discImageDevice.read($0, position, $3);
	},
	                   buffer, positionLow, positionHigh, static_cast<uint32>(size));
	while(!MAIN_THREAD_EM_ASM_INT({return Module.discImageDevice.isDone()}))
	{
		usleep(100);
	}
	m_position += size;
	return size;
}

uint64 CJsDiscImageDeviceStream::Write(const void*, uint64)
{
	throw std::runtime_error("Not supported.");
}

bool CJsDiscImageDeviceStream::IsEOF()
{
	throw std::runtime_error("Not supported.");
}

void CJsDiscImageDeviceStream::Flush()
{
}
