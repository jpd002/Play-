#pragma once

#include <memory>
#include "Ioman_Device.h"
#include "../OpticalMedia.h"

namespace Iop
{
	namespace Ioman
	{
		class COpticalMediaDevice : public CDevice
		{
		public:
			typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;

			        COpticalMediaDevice(OpticalMediaPtr&);
			virtual ~COpticalMediaDevice() = default;

			Framework::CStream* GetFile(uint32, const char*) override;

		private:
			static char FixSlashes(char);

			OpticalMediaPtr& m_opticalMedia;
		};
	}
}
