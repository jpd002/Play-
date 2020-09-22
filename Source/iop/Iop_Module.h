#pragma once

#include <string>
#include <memory>
#include "../MIPS.h"

namespace Framework
{
	class CZipArchiveWriter;
	class CZipArchiveReader;
};

namespace Iop
{
	class CModule
	{
	public:
		virtual ~CModule() = default;
		virtual std::string GetId() const = 0;
		virtual std::string GetFunctionName(unsigned int) const = 0;
		virtual void Invoke(CMIPS&, unsigned int) = 0;

		virtual void SaveState(Framework::CZipArchiveWriter&) const {};
		virtual void LoadState(Framework::CZipArchiveReader&){};

		static std::string PrintStringParameter(const uint8*, uint32);
	};

	typedef std::shared_ptr<CModule> ModulePtr;
};
