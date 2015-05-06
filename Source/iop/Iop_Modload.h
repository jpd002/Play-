#pragma once

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CModload : public CModule
	{
	public:
						CModload(CIopBios&, uint8*);
		virtual			~CModload();

		std::string		GetId() const override;
		std::string		GetFunctionName(unsigned int) const override;
		void			Invoke(CMIPS&, unsigned int) override;

	private:
		uint32			LoadStartModule(uint32, uint32, uint32, uint32);
		uint32			StartModule(uint32, uint32, uint32, uint32, uint32);
		uint32			LoadModuleBuffer(uint32);
		uint32			GetModuleIdList(uint32, uint32, uint32);

		CIopBios&		m_bios;
		uint8*			m_ram = nullptr;
	};
}
