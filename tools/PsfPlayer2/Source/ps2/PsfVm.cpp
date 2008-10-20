#include "PsfVm.h"
#include "Ps2Const.h"
#include "MA_MIPSIV.h"

using namespace PS2;
using namespace std;
using namespace std::tr1;

#define PSF_DEVICENAME "psf"

CPsfVm::CPsfVm() :
m_ram(new uint8[IOPRAMSIZE]),
m_iop(MEMORYMAP_ENDIAN_LSBF, 0x00000000, IOPRAMSIZE),
m_bios(0x1000, m_iop, m_ram, IOPRAMSIZE, *reinterpret_cast<CSIF*>(NULL), *reinterpret_cast<CISO9660**>(NULL))
{
    //IOP context setup
    {
        //Read map
	    m_iop.m_pMemoryMap->InsertReadMap(0x00000000, IOPRAMSIZE - 1, m_ram,         0x00);

        //Write map
        m_iop.m_pMemoryMap->InsertWriteMap(0x00000000, IOPRAMSIZE -1, m_ram,		0x00);

	    //Instruction map
        m_iop.m_pMemoryMap->InsertInstructionMap(0x00000000, IOPRAMSIZE - 1, m_ram,  0x00);

	    m_iop.m_pArch			= &g_MAMIPSIV;
    }
}

CPsfVm::~CPsfVm()
{

}

CDebuggable CPsfVm::GetIopDebug() const
{
    CDebuggable debug;
    debug.GetCpu = bind(&CPsfVm::GetCpu, this);
    return debug;
}

void CPsfVm::LoadPsf(const CPsfDevice::PsfFile& psfFile)
{
    Iop::CIoman* ioman = m_bios.GetIoman();
    ioman->RegisterDevice(PSF_DEVICENAME, new CPsfDevice(psfFile));

    string execPath = string(PSF_DEVICENAME) + ":/psf2.irx";
    m_bios.LoadAndStartModule(execPath.c_str(), NULL, 0);
}

void CPsfVm::Reset()
{
    memset(m_ram, 0, IOPRAMSIZE);
    m_bios.Reset();
}

CVirtualMachine::STATUS CPsfVm::GetStatus() const
{
    return PAUSED;
}

void CPsfVm::Resume()
{

}

void CPsfVm::Pause()
{

}

CMIPS& CPsfVm::GetCpu()
{
    return m_iop;
}
