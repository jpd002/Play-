#include <cstring>
#include "string_format.h"
#include "Iop_Dynamic.h"
#include "Log.h"

#define LOG_NAME ("iop_dynamic")

using namespace Iop;

static constexpr uint32 g_exportModuleNameOffset = 3;
static constexpr uint32 g_exportFctTableOffset = 5;

CDynamic::CDynamic(const uint32* exportTable)
    : m_exportTable(exportTable)
{
	m_name = GetDynamicModuleName(exportTable);
	m_functionCount = GetDynamicModuleExportCount(exportTable);
}

std::string CDynamic::GetDynamicModuleName(const uint32* exportTable)
{
	//Name is 8 bytes long without zero, so we need to make sure it's properly null-terminated
	const unsigned int nameLength = 8;
	char name[nameLength + 1];
	memset(name, 0, nameLength + 1);
	memcpy(name, reinterpret_cast<const char*>(exportTable + g_exportModuleNameOffset), nameLength);
	return name;
}

uint32 CDynamic::GetDynamicModuleExportCount(const uint32* exportTable)
{
	//Export tables are supposed to finish with a 0 value.
	uint32 functionCount = 0;
	while(exportTable[g_exportFctTableOffset + functionCount] != 0)
	{
		functionCount++;
		if(functionCount >= 1000)
		{
			CLog::GetInstance().Warn(LOG_NAME, "Export count exceeded threshold of %d functions. Bailing.\r\n", functionCount);
			break;
		}
	}
	return functionCount;
}

std::string CDynamic::GetId() const
{
	return m_name;
}

std::string CDynamic::GetFunctionName(unsigned int functionId) const
{
	return string_format("unknown_%04X", functionId);
}

void CDynamic::Invoke(CMIPS& context, unsigned int functionId)
{
	if(functionId < m_functionCount)
	{
		uint32 functionAddress = m_exportTable[g_exportFctTableOffset + functionId];
		context.m_State.nGPR[CMIPS::RA].nD0 = context.m_State.nPC;
		context.m_State.nPC = functionAddress;
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, "Failed to find export %d for module '%s'.\r\n",
		                         functionId, m_name.c_str());
	}
}

const uint32* CDynamic::GetExportTable() const
{
	return m_exportTable;
}
