#pragma once

#include "Iop_Module.h"

namespace Iop
{
	class CIoman;

	class CIomanX : public CModule
	{
	public:
		CIomanX(CIoman&);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SaveState(Framework::CZipArchiveWriter&) const override;
		void LoadState(Framework::CZipArchiveReader&) override;

	private:
		CIoman& m_ioman;
	};
}
