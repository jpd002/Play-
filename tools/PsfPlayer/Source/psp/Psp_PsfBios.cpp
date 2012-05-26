#include "Psp_PsfBios.h"

using namespace Psp;

CPsfBios::CPsfBios(CMIPS& cpu, uint8* ram, uint32 ramSize)
: m_bios(cpu, ram, ramSize)
{
	Reset();
}

CPsfBios::~CPsfBios()
{

}

void CPsfBios::AppendArchive(const CPsfBase& psfFile)
{
	m_psfDevice->AppendArchive(psfFile);
}

CSasCore* CPsfBios::GetSasCore()
{
	return m_bios.GetSasCore();
}

CAudio* CPsfBios::GetAudio()
{
	return m_bios.GetAudio();
}

void CPsfBios::Reset()
{
	m_bios.Reset();

	//Initialize Io devices
	m_psfDevice = PsfDevicePtr(new CPsfDevice());
	m_bios.GetIoFileMgr()->RegisterDevice("host0", m_psfDevice);
}

void CPsfBios::Start()
{
	m_bios.LoadModule("host0:/PSP_GAME/SYSDIR/BOOT.BIN");
}

void CPsfBios::HandleException()
{
	m_bios.HandleException();
}

void CPsfBios::HandleInterrupt()
{

}

void CPsfBios::CountTicks(uint32 ticks)
{

}

bool CPsfBios::IsIdle()
{
	return false;
}

BiosDebugModuleInfoArray CPsfBios::GetModuleInfos() const
{
	return m_bios.GetModuleInfos();
}

BiosDebugThreadInfoArray CPsfBios::GetThreadInfos() const
{
	return m_bios.GetThreadInfos();
}

#ifdef DEBUGGER_INCLUDED

void CPsfBios::LoadDebugTags(Framework::Xml::CNode* root)
{

}

void CPsfBios::SaveDebugTags(Framework::Xml::CNode* root)
{

}

#endif
