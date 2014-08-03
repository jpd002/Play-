#pragma once

#include "Iop_Module.h"
#include "ArgumentIterator.h"

namespace Iop
{
	class CIoman;

	class CStdio : public CModule
	{
	public:
						CStdio(uint8*, CIoman&);
		virtual			~CStdio();

		std::string		GetId() const override;
		std::string		GetFunctionName(unsigned int) const override;
		void			Invoke(CMIPS&, unsigned int) override;

		void			__printf(CMIPS&);
		std::string		PrintFormatted(CArgumentIterator&);

	private:
		uint8*			m_ram;
		CIoman&			m_ioman;
	};
}
