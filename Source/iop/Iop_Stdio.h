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
		virtual ~CStdio() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void __printf(CMIPS&);
		int32 __puts(uint32);
		std::string PrintFormatted(const char*, CArgumentIterator&);

	private:
		uint8* m_ram;
		CIoman& m_ioman;
	};

	typedef std::shared_ptr<CStdio> StdioPtr;
}
