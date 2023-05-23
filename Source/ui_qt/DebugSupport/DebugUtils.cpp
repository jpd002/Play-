#include "DebugUtils.h"
#include "string_cast.h"
#include "string_format.h"
#include "MIPS.h"
#include "DebugSupportSettings.h"

#define FIND_MAX_ADDRESS 0x02000000

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

std::vector<uint32> DebugUtils::FindCallers(CMIPS* context, uint32 address)
{
	std::vector<uint32> callers;
	for(uint32 i = 0; i < FIND_MAX_ADDRESS; i += 4)
	{
		uint32 opcode = context->m_pMemoryMap->GetInstruction(i);
		uint32 ea = context->m_pArch->GetInstructionEffectiveAddress(context, i, opcode);
		if(ea == MIPS_INVALID_PC) continue;
		if(ea == address)
		{
			callers.push_back(i);
		}
	}
	return callers;
}

std::vector<uint32> DebugUtils::FindWordValueRefs(CMIPS* context, uint32 targetValue, uint32 valueMask)
{
	std::vector<uint32> refs;
	for(uint32 i = 0; i < FIND_MAX_ADDRESS; i += 4)
	{
		uint32 valueAtAddress = context->m_pMemoryMap->GetWord(i);
		if((valueAtAddress & valueMask) == targetValue)
		{
			refs.push_back(i);
		}
	}
	return refs;
}

QFont DebugUtils::CreateMonospaceFont()
{
	auto fontFaceName = CDebugSupportSettings::GetInstance().GetMonospaceFontFaceName();
	auto fontSize = CDebugSupportSettings::GetInstance().GetMonospaceFontSize();

	return QFont(fontFaceName.c_str(), fontSize);
}
