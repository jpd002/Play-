#include "PsfBios.h"

using namespace PS2;

#define PSF_DEVICENAME	"psf"

CPsfBios::CPsfBios(CMIPS& cpu, uint8* ram, uint32 ramSize) 
: m_bios(cpu, ram, ramSize)
, m_psfDevice(new CPsfDevice())
{
	m_bios.Reset(NULL);

	Iop::CIoman* ioman = m_bios.GetIoman();
	ioman->RegisterDevice(PSF_DEVICENAME,	m_psfDevice);
	ioman->RegisterDevice("host0",			m_psfDevice);
	ioman->RegisterDevice("hefile",			m_psfDevice);
}

CPsfBios::~CPsfBios()
{

}

void CPsfBios::AppendArchive(const CPsfBase& psfFile)
{
	static_cast<CPsfDevice*>(m_psfDevice.get())->AppendArchive(psfFile);
}

void CPsfBios::Start()
{
	std::string execPath = std::string(PSF_DEVICENAME) + ":/psf2.irx";
	auto moduleId = m_bios.LoadModule(execPath.c_str());
	assert(moduleId >= 0);
	m_bios.StartModule(moduleId, execPath.c_str(), nullptr, 0);
}

void CPsfBios::HandleException()
{
	m_bios.HandleException();
}

void CPsfBios::HandleInterrupt()
{
	m_bios.HandleInterrupt();
}

void CPsfBios::CountTicks(uint32 ticks)
{
	m_bios.CountTicks(ticks);
}

void CPsfBios::SaveState(Framework::CZipArchiveWriter& archive)
{

}

void CPsfBios::LoadState(Framework::CZipArchiveReader& archive)
{

}

void CPsfBios::NotifyVBlankStart()
{
	m_bios.NotifyVBlankStart();
}

void CPsfBios::NotifyVBlankEnd()
{
	m_bios.NotifyVBlankEnd();
}

bool CPsfBios::IsIdle()
{
	return m_bios.IsIdle();
}

#ifdef DEBUGGER_INCLUDED

void CPsfBios::LoadDebugTags(Framework::Xml::CNode* root)
{
	m_bios.LoadDebugTags(root);
}

void CPsfBios::SaveDebugTags(Framework::Xml::CNode* root)
{
	m_bios.SaveDebugTags(root);
}

BiosDebugModuleInfoArray CPsfBios::GetModulesDebugInfo() const
{
	return m_bios.GetModulesDebugInfo();
}

BiosDebugThreadInfoArray CPsfBios::GetThreadsDebugInfo() const
{
	return m_bios.GetThreadsDebugInfo();
}

#endif
