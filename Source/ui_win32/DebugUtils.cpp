#include "DebugUtils.h"
#include "lexical_cast_ex.h"
#include "string_cast.h"

std::tstring DebugUtils::PrintAddressLocation(uint32 address, CMIPS* context, const BiosDebugModuleInfoArray& modules)
{
	std::tstring locationString = _T("0x") + lexical_cast_hex<std::tstring>(address, 8);

	auto module = FindModuleAtAddress(modules, address);
	const char* functionName = nullptr;
	if(auto subroutine = context->m_analysis->FindSubroutine(address))
	{
		functionName = context->m_Functions.Find(subroutine->start);
	}
	bool hasParenthesis = (functionName != nullptr) || (module != nullptr);
	if(hasParenthesis)
	{
		locationString += _T(" (");
	}
	if(functionName)
	{
		locationString += string_cast<std::tstring>(functionName);
	}
	if((functionName != nullptr) && (module != nullptr))
	{
		locationString += _T(" : ");
	}
	if(module)
	{
		locationString += string_cast<std::tstring>(module->name);
	}
	if(hasParenthesis)
	{
		locationString += _T(")");
	}
	return locationString;
}

const BIOS_DEBUG_MODULE_INFO* DebugUtils::FindModuleAtAddress(const BiosDebugModuleInfoArray& modules, uint32 address)
{
	for(auto moduleIterator(std::begin(modules));
		moduleIterator != std::end(modules); moduleIterator++)
	{
		const auto& module = (*moduleIterator);
		if(address >= module.begin && address < module.end)
		{
			return &module;
		}
	}
	return nullptr;
}
