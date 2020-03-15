#pragma once

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CSecrman : public CModule
	{
	public:
		CSecrman() = default;
		virtual ~CSecrman() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		void SetMcCommandHandler(uint32);
		void SetMcDevIdHandler(uint32);
		uint32 AuthCard(uint32, uint32, uint32);
	};
}
