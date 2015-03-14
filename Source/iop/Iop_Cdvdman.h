#pragma once

#include "Iop_Module.h"
#include "../ISO9660/ISO9660.h"

class CIopBios;

namespace Iop
{
	class CCdvdman : public CModule
	{
	public:
								CCdvdman(CIopBios&, uint8*);
		virtual					~CCdvdman();

		virtual std::string		GetId() const override;
		virtual std::string		GetFunctionName(unsigned int) const override;
		virtual void			Invoke(CMIPS&, unsigned int) override;

		void					SetIsoImage(CISO9660*);

	private:
		uint32					CdRead(uint32, uint32, uint32, uint32);
		uint32					CdSeek(uint32);
		uint32					CdGetError();
		uint32					CdSearchFile(uint32, uint32);
		uint32					CdSync(uint32);
		uint32					CdGetDiskType();
		uint32					CdDiskReady(uint32);
		uint32					CdReadClock(uint32);
		uint32					CdStatus();
		uint32					CdCallback(uint32);

		CIopBios&				m_bios;
		CISO9660*				m_image = nullptr;
		uint8*					m_ram = nullptr;

		//TODO: Save state of this
		uint32					m_callbackPtr = 0;
	};
};
