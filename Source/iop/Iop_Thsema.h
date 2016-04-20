#pragma once

#include "Iop_Module.h"
#include "IopBios.h"

namespace Iop
{
	class CThsema : public CModule
	{
	public:
						CThsema(CIopBios&, uint8*);
		virtual			~CThsema();

		std::string		GetId() const override;
		std::string		GetFunctionName(unsigned int) const override;
		void			Invoke(CMIPS&, unsigned int) override;

	private:
		struct SEMAPHORE
		{
			uint32		attributes;
			uint32		options;
			uint32		initialCount;
			uint32		maxCount;
		};
		
		uint32			CreateSemaphore(const SEMAPHORE*);
		uint32			DeleteSemaphore(uint32);
		uint32			WaitSemaphore(uint32);
		uint32			PollSemaphore(uint32);
		uint32			SignalSemaphore(uint32);
		uint32			iSignalSemaphore(uint32);
		uint32			ReferSemaphoreStatus(uint32, uint32);

		uint8*			m_ram;
		CIopBios&		m_bios;
	};
}
