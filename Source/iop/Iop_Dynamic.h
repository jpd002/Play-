#pragma once

#include "Iop_Module.h"

namespace Iop
{
	class CDynamic : public CModule
	{
	public:
		CDynamic(const uint32*);
		virtual ~CDynamic() = default;

		static std::string GetDynamicModuleName(const uint32*);
		static uint32 GetDynamicModuleExportCount(const uint32*);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		const uint32* GetExportTable() const;

	private:
		const uint32* m_exportTable;
		std::string m_name;
		uint32 m_functionCount = 0;
	};

	typedef std::shared_ptr<CDynamic> DynamicPtr;
}
