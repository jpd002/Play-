#include "Iop_Dynamic.h"

using namespace Iop;

CDynamic::CDynamic(uint32* exportTable) 
: m_exportTable(exportTable)
, m_name(reinterpret_cast<char*>(m_exportTable) + 12)
{

}

CDynamic::~CDynamic()
{

}

std::string CDynamic::GetId() const
{
	return m_name;
}

std::string CDynamic::GetFunctionName(unsigned int functionId) const
{
	char functionName[256];
	sprintf(functionName, "unknown_%0.4X", functionId);
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
