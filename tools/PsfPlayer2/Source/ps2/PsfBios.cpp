#include "PsfBios.h"

using namespace PS2;
using namespace std;

#define PSF_DEVICENAME	"psf"

CPsfBios::CPsfBios(const CPsfDevice::PsfFile& psfFile, CMIPS& cpu, uint8* ram, uint32 ramSize) :
m_bios(0x1000, cpu, ram, ramSize, *reinterpret_cast<CSIF*>(NULL), *reinterpret_cast<CISO9660**>(NULL))
{
	m_bios.Reset();

    Iop::CIoman* ioman = m_bios.GetIoman();
    ioman->RegisterDevice(PSF_DEVICENAME, new CPsfDevice(psfFile));

    string execPath = string(PSF_DEVICENAME) + ":/psf2.irx";
    m_bios.LoadAndStartModule(execPath.c_str(), NULL, 0);
}

CPsfBios::~CPsfBios()
{

}

void CPsfBios::HandleException()
{
	m_bios.HandleException();
}

void CPsfBios::HandleInterrupt()
{
	m_bios.HandleInterrupt();
}
