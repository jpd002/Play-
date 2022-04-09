#include "DebugUtils.h"
#include "string_cast.h"
#include "string_format.h"
#include "DebugSupportSettings.h"

std::string DebugUtils::PrintAddressLocation(uint32 address, CMIPS* context, const BiosDebugModuleInfoArray& modules)
{
	auto locationString = string_format(("0x%08X"), address);

	auto module = FindModuleAtAddress(modules, address);
	const char* functionName = nullptr;
	if(auto subroutine = context->m_analysis->FindSubroutine(address))
	{
		functionName = context->m_Functions.Find(subroutine->start);
	}
	bool hasParenthesis = (functionName != nullptr) || (module != nullptr);
	if(hasParenthesis)
	{
		locationString += (" (");
	}
	if(functionName)
	{
		locationString += functionName;
	}
	if((functionName != nullptr) && (module != nullptr))
	{
		locationString += (" : ");
	}
	if(module)
	{
		locationString += module->name;
	}
	if(hasParenthesis)
	{
		locationString += (")");
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

QFont DebugUtils::CreateMonospaceFont()
{
	auto fontFaceName = CDebugSupportSettings::GetInstance().GetMonospaceFontFaceName();
	auto fontSize = CDebugSupportSettings::GetInstance().GetMonospaceFontSize();

	return QFont(fontFaceName.c_str(), fontSize);
}
