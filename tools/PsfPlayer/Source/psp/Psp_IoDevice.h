#pragma once

#include <memory>
#include "Stream.h"

namespace Psp
{
	class CIoDevice
	{
	public:
		virtual ~CIoDevice() = default;
		virtual Framework::CStream* GetFile(const char*, uint32) = 0;
	};

	typedef std::shared_ptr<CIoDevice> IoDevicePtr;
}
