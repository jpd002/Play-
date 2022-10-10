#pragma once

#include "../Iop_Module.h"
#include "../Iop_SifMan.h"
#include "../../OpticalMedia.h"

namespace Iop
{
	class CCdvdman;

	namespace Namco
	{
		class CAcCdvd : public CModule, public CSifModule
		{
		public:
			CAcCdvd(CSifMan&, CCdvdman&, uint8*);
			virtual ~CAcCdvd() = default;

			void SetOpticalMedia(COpticalMedia*);

			std::string GetId() const override;
			std::string GetFunctionName(unsigned int) const override;
			void Invoke(CMIPS&, unsigned int) override;
			bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		private:
			enum
			{
				MODULE_ID = 0x76500002,
			};
			
			CCdvdman& m_cdvdman;
			uint8* m_iopRam = nullptr;
			COpticalMedia* m_opticalMedia = nullptr;

			uint32 m_streamPos = 0;
		};
	}
}
