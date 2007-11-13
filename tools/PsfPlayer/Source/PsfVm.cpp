#include "PsfVm.h"
#include "MA_MIPSIV.h"
#include "COP_SCU.h"
#include "PtrStream.h"
#include "StdStream.h"
#include "Ioman_Psf.h"
#include "ELF.h"
#include <boost/bind.hpp>
#include <boost/filesystem/path.hpp>

using namespace Framework;
using namespace std;
using namespace boost;

#define PSF_DEVICENAME "psf"

CPsfVm::CPsfVm(const char* psfPath) :
m_fileSystem(psfPath),
m_cpu(MEMORYMAP_ENDIAN_LSBF, 0x00000000, RAMSIZE),
m_status(PAUSED),
m_pauseAck(false),
m_emuThread(NULL),
m_bios(NULL),
m_singleStep(false)
{
    memset(&m_cpu.m_State, 0, sizeof(m_cpu.m_State));
    m_ram = new uint8[RAMSIZE];
	m_cpu.m_pMemoryMap->InsertReadMap(0x00000000, RAMSIZE - 1, m_ram, MEMORYMAP_TYPE_MEMORY, 0x00);
    m_cpu.m_pMemoryMap->InsertWriteMap(0x00000000, RAMSIZE - 1, m_ram, MEMORYMAP_TYPE_MEMORY, 0x00);

    m_cpu.m_pArch = &g_MAMIPSIV;
    m_cpu.m_pAddrTranslator = m_cpu.TranslateAddress64;
    m_cpu.m_pTickFunction = reinterpret_cast<TickFunctionType>(TickFunctionStub);
    m_cpu.m_pSysCallHandler = reinterpret_cast<SysCallHandlerType>(SysCallHandlerStub);
    m_cpu.m_handlerParam = this;

#ifdef _DEBUG
    m_cpu.m_Functions.Unserialize("functions.bin");
    m_cpu.m_Comments.Unserialize("comments.bin");
#endif

    m_bios = new CIopBios(0x100, m_cpu, m_ram, RAMSIZE);

    string execPath = string(PSF_DEVICENAME) + ":/psf2.irx";

    m_bios->GetIoman()->RegisterDevice(PSF_DEVICENAME, new Iop::Ioman::CPsf(m_fileSystem));
    m_bios->LoadAndStartModule(execPath.c_str(), NULL, 0);
/*
    {
        uint32 handle = m_bios->GetIoman()->Open(Iop::Ioman::CDevice::O_RDONLY, 
            (PSF_DEVICENAME + string(":/IOPSOUND_linkfix.IRX")).c_str());
        CStream* stream = m_bios->GetIoman()->GetFileStream(handle);
        stream->Seek(0, STREAM_SEEK_END);
        uint64 size = stream->Tell();
        stream->Seek(0, STREAM_SEEK_SET);
        uint8* buffer = new uint8[size];
        stream->Read(buffer, size);
        m_bios->GetIoman()->Close(handle);
        CStdStream output("iopsound_linkfix.irx", "wb");
        output.Write(buffer, size);
    }
*/
    m_emuThread = new thread(bind(&CPsfVm::EmulationProc, this));
}

CPsfVm::~CPsfVm()
{
#ifdef _DEBUG
    m_cpu.m_Functions.Serialize("functions.bin");
    m_cpu.m_Comments.Serialize("comments.bin");
#endif
    delete m_bios;
    delete [] m_ram;
}

void CPsfVm::Pause()
{
    if(GetStatus() == PAUSED) return;
    m_pauseAck = false;
    m_status = PAUSED;
    while(!m_pauseAck)
    {
        xtime xt;
        xtime_get(&xt, boost::TIME_UTC);
        xt.nsec += 100000000;
    }
    m_OnRunningStateChange();
}

void CPsfVm::Step()
{
    if(GetStatus() == RUNNING) return;
    m_singleStep = true;
    m_status = RUNNING;
}

void CPsfVm::Resume()
{
    if(GetStatus() == RUNNING) return;
    m_status = RUNNING;
    m_OnRunningStateChange();
}

CMIPS& CPsfVm::GetCpu()
{
    return m_cpu;
}

CVirtualMachine::STATUS CPsfVm::GetStatus() const
{
    return m_status;
}

unsigned int CPsfVm::TickFunctionStub(unsigned int ticks, CMIPS* context)
{
    return reinterpret_cast<CPsfVm*>(context->m_handlerParam)->TickFunction(ticks);
}

unsigned int CPsfVm::TickFunction(unsigned int ticks)
{
    if(m_cpu.MustBreak())
    {
        return 1;
    }
    return 0;
}

void CPsfVm::SysCallHandlerStub(CMIPS* state)
{
    reinterpret_cast<CPsfVm*>(state->m_handlerParam)->m_bios->SysCallHandler();
}

void CPsfVm::EmulationProc()
{
    while(1)
    {
        if(m_status == RUNNING)
        {
            RET_CODE returnCode = m_cpu.Execute(m_singleStep ? 1 : 1000);
            if(m_cpu.m_State.nCOP0[CCOP_SCU::EPC] != 0)
            {
                m_bios->SysCallHandler();
                assert(m_cpu.m_State.nCOP0[CCOP_SCU::EPC] == 0);
            }
            if(returnCode == RET_CODE_BREAKPOINT || m_singleStep)
            {
                m_status = PAUSED;
                m_singleStep = false;
                m_OnRunningStateChange();
                m_OnMachineStateChange();
            }
        }
        else
        {
            m_pauseAck = true;
            xtime xt;
            xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 10000000;
            thread::sleep(xt);
        }
    }
}
