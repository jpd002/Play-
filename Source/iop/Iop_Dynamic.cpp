#include <cstring>
#include "Iop_Dynamic.h"

using namespace Iop;

CDynamic::CDynamic(uint32* exportTable)
    : m_exportTable(exportTable)
{
	m_name = GetDynamicModuleName(exportTable);
}

std::string CDynamic::GetDynamicModuleName(uint32* exportTable)
{
	//Name is 8 bytes long without zero, so we need to make sure it's properly null-terminated
	const unsigned int nameLength = 8;
	char name[nameLength + 1];
	memset(name, 0, nameLength + 1);
	memcpy(name, reinterpret_cast<const char*>(exportTable) + 12, nameLength);
	return name;
}

std::string CDynamic::GetId() const
{
	return m_name;
}

std::string CDynamic::GetFunctionName(unsigned int functionId) const
{
	char functionName[256];
	sprintf(functionName, "unknown_%04X", functionId);
	return functionName;
}

void CDynamic::Invoke(CMIPS& context, unsigned int functionId)
{
	uint32 functionAddress = m_exportTable[5 + functionId];
	context.m_State.nGPR[CMIPS::RA].nD0 = context.m_State.nPC;
	context.m_State.nPC = functionAddress;
}

uint32* CDynamic::GetExportTable() const
{
	return m_exportTable;
}
