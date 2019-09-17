#include "make_unique.h"
#include "Iop_FileIo.h"
#include "Iop_FileIoHandler1000.h"
#include "Iop_FileIoHandler2100.h"
#include "Iop_FileIoHandler2240.h"
#include "../states/RegisterStateFile.h"

#define LOG_NAME ("iop_fileio")

#define STATE_VERSION_XML ("iop_fileio/version.xml")
#define STATE_VERSION_MODULEVERSION ("moduleVersion")

const char* Iop::CFileIo::g_moduleId = "fileio";

using namespace Iop;

CFileIo::CFileIo(CIopBios& bios, uint8* ram, CSifMan& sifMan, CIoman& ioman)
    : m_bios(bios)
    , m_ram(ram)
    , m_sifMan(sifMan)
    , m_ioman(ioman)
{
	m_sifMan.RegisterModule(SIF_MODULE_ID, this);
	m_handler = std::make_unique<CFileIoHandler1000>(m_bios, m_ram, &m_ioman, m_sifMan);
}

void CFileIo::SetModuleVersion(unsigned int moduleVersion)
{
	m_handler.reset();
	m_moduleVersion = moduleVersion;
	if(moduleVersion >= 2100 && moduleVersion < 2240)
	{
		m_handler = std::make_unique<CFileIoHandler2100>(&m_ioman);
	}
	else if(moduleVersion >= 2240)
	{
		m_handler = std::make_unique<CFileIoHandler2240>(&m_ioman, m_sifMan);
	}
	else
	{
		m_handler = std::make_unique<CFileIoHandler1000>(m_bios, m_ram, &m_ioman, m_sifMan);
	}
}

std::string CFileIo::GetId() const
{
	return g_moduleId;
}

std::string CFileIo::GetFunctionName(unsigned int) const
{
	return "unknown";
}

//IOP Invoke
void CFileIo::Invoke(CMIPS& context, unsigned int functionId)
{
	m_handler->Invoke(context, functionId);
}

//SIF Invoke
bool CFileIo::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	return m_handler->Invoke(method, args, argsSize, ret, retSize, ram);
}

void CFileIo::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_VERSION_XML));
	m_moduleVersion = registerFile.GetRegister32(STATE_VERSION_MODULEVERSION);
	SetModuleVersion(m_moduleVersion);
	m_handler->LoadState(archive);
}

void CFileIo::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = new CRegisterStateFile(STATE_VERSION_XML);
	registerFile->SetRegister32(STATE_VERSION_MODULEVERSION, m_moduleVersion);
	archive.InsertFile(registerFile);
	m_handler->SaveState(archive);
}

void CFileIo::ProcessCommands(Iop::CSifMan* sifMan)
{
	m_handler->ProcessCommands(sifMan);
}

//--------------------------------------------------
// CHandler
//--------------------------------------------------

CFileIo::CHandler::CHandler(CIoman* ioman)
    : m_ioman(ioman)
{
}

void CFileIo::CHandler::Invoke(CMIPS&, uint32)
{
	throw std::exception();
}
