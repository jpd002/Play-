#include "Iop_FileIo.h"
#include <cstring>
#include "make_unique.h"
#include "Iop_FileIoHandler1000.h"
#include "Iop_FileIoHandler2100.h"
#include "Iop_FileIoHandler2200.h"
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
	SetModuleVersion(1000);
}

void CFileIo::SetModuleVersion(unsigned int moduleVersion)
{
	if(m_handler)
	{
		m_handler->ReleaseMemory();
	}
	m_moduleVersion = moduleVersion;
	SyncHandler();
	m_handler->AllocateMemory();
}

void CFileIo::SyncHandler()
{
	//Honorable mentions regarding FileIO versions:
	//- FileIO version 2050 seems to be working the same as version 1000 (used by Piposaru 2001).
	//- Maximo: Ghosts to Glory attempts to use EE lib version 2200 with stock BIOS version 1000 (this is assumed to fail).
	//- Gran Turismo 4 will uses version 1000 before rebooting IOP with a newer version.
	m_handler.reset();
	if(m_moduleVersion >= 2100 && m_moduleVersion < 2200)
	{
		m_handler = std::make_unique<CFileIoHandler2100>(&m_ioman);
	}
	else if(m_moduleVersion >= 2200)
	{
		m_handler = std::make_unique<CFileIoHandler2200>(&m_ioman, m_sifMan);
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
	if(method == 255)
	{
		//Init
		//All FileIO modules have this SIF RPC call id. We need to properly report the version number currently
		//in the IOP to make sure we avoid some mismatch between EE/IOP versions.
		assert((m_moduleVersion >= 1000) && (m_moduleVersion <= 9999));
		char versionString[16] = {};
		if(retSize == 8)
		{
			sprintf(versionString, "%d....", m_moduleVersion);
			memcpy(ret, versionString, 8);
		}
		else if(retSize == 4)
		{
			sprintf(versionString, "%d", m_moduleVersion);
			memcpy(ret, versionString, 4);
		}
		else
		{
			assert(0);
		}
	}
	return m_handler->Invoke(method, args, argsSize, ret, retSize, ram);
}

void CFileIo::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_VERSION_XML));
	m_moduleVersion = registerFile.GetRegister32(STATE_VERSION_MODULEVERSION);
	SyncHandler();
	m_handler->LoadState(archive);
}

void CFileIo::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_VERSION_XML);
	registerFile->SetRegister32(STATE_VERSION_MODULEVERSION, m_moduleVersion);
	archive.InsertFile(std::move(registerFile));
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
