#pragma once

#include "Iop_SifMan.h"

namespace Iop
{
	class CSifCmd;

	class CSifDynamic : public CSifModule
	{
	public:
						CSifDynamic(CSifCmd&, uint32);
		virtual			~CSifDynamic();
		bool			Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		uint32			GetServerDataAddress() const;

	private:
		CSifCmd&		m_sifCmd;
		uint32			m_serverDataAddress = 0;
	};
}
