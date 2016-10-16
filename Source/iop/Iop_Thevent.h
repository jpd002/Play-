#pragma once

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CThevent : public CModule
	{
	public:
						CThevent(CIopBios&, uint8*);
		virtual			~CThevent();

		std::string		GetId() const override;
		std::string		GetFunctionName(unsigned int) const override;
		void			Invoke(CMIPS&, unsigned int) override;

	private:
		struct EVENT
		{
			uint32		attributes;
			uint32		options;
			uint32		initValue;
		};

		uint32			CreateEventFlag(EVENT*);
		uint32			DeleteEventFlag(uint32);
		uint32			SetEventFlag(uint32, uint32);
		uint32			iSetEventFlag(uint32, uint32);
		uint32			ClearEventFlag(uint32, uint32);
		uint32			WaitEventFlag(uint32, uint32, uint32, uint32);
		uint32			ReferEventFlagStatus(uint32, uint32);

		uint8*			m_ram = nullptr;
		CIopBios&		m_bios;
	};
}
