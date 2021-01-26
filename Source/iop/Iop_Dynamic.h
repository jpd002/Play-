#pragma once

#include "Iop_Module.h"

namespace Iop
{
	class CDynamic : public CModule
	{
	public:
		CDynamic(uint32*);
		virtual ~CDynamic() = default;

		static std::string GetDynamicModuleName(uint32*);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		uint32* GetExportTable() const;
		
	private:
		uint32* m_exportTable;
		std::string m_name;
	};

	typedef std::shared_ptr<CDynamic> DynamicPtr;
}
