#include "Iop_Module.h"
#include "string_format.h"

using namespace Iop;

std::string CModule::PrintStringParameter(const uint8* ram, uint32 stringPtr)
{
	auto result = string_format("0x%08X", stringPtr);
	if(stringPtr != 0)
	{
		auto string = reinterpret_cast<const char*>(ram + stringPtr);
		result += string_format(" ('%s')", string);
	}
	return std::move(result);
}
