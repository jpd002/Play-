#pragma once

#include <memory>
#include "Ioman_Device.h"
#include "../ISO9660/ISO9660.h"

namespace Iop
{
	namespace Ioman
	{
		class CIsoDevice : public CDevice
		{
		public:
			typedef std::unique_ptr<CISO9660> Iso9660Ptr;

											CIsoDevice(Iso9660Ptr&);
			virtual							~CIsoDevice();

			virtual Framework::CStream*		GetFile(uint32, const char*) override;

		private:
			static char						FixSlashes(char);

			Iso9660Ptr&						m_iso;
		};
	}
}
