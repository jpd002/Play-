#include "Iop_FileIo.h"
#include "Iop_FileIoHandler1000.h"
#include "Iop_FileIoHandler2100.h"
#include "Iop_FileIoHandler2300.h"

#define LOG_NAME ("iop_fileio")

using namespace Iop;

CFileIo::CFileIo(CSifMan& sifMan, CIoman& ioman) 
: m_sifMan(sifMan)
, m_ioman(ioman)
{
	m_sifMan.RegisterModule(SIF_MODULE_ID, this);
	m_handler = std::make_unique<CFileIoHandler1000>(&m_ioman);
}

CFileIo::~CFileIo()
{

}

void CFileIo::SetModuleVersion(unsigned int moduleVersion)
{
	m_handler.reset();
	if(moduleVersion >= 2100 && moduleVersion < 2300)
	{
		m_handler = std::make_unique<CFileIoHandler2100>(&m_ioman);
	}
	else if(moduleVersion >= 2300)
	{
		m_handler = std::make_unique<CFileIoHandler2300>(&m_ioman, m_sifMan);
	}
	else
	{
		m_handler = std::make_unique<CFileIoHandler1000>(&m_ioman);
	}
}

std::string CFileIo::GetId() const
{
	return "fileio";
}

std::string CFileIo::GetFunctionName(unsigned int) const
{
	return "unknown";
}

//IOP Invoke
void CFileIo::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::exception();
}

//SIF Invoke
bool CFileIo::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	m_handler->Invoke(method, args, argsSize, ret, retSize, ram);
	return true;
}

//--------------------------------------------------
// CHandler
//--------------------------------------------------

CFileIo::CHandler::CHandler(CIoman* ioman)
: m_ioman(ioman)
{

}
