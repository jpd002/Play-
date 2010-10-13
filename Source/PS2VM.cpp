#include <stdio.h>
#include <exception>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <memory>
#include "PS2VM.h"
#include "PS2VM_Preferences.h"
#include "PS2OS.h"
#include "Ps2Const.h"
#include "iop/Iop_SifManPs2.h"
#include "VIF.h"
#include "Timer.h"
#include "PtrMacro.h"
#include "StdStream.h"
#include "GZipStream.h"
#ifdef WIN32
#include "VolumeStream.h"
#else
#include "Posix_VolumeStream.h"
#endif
#include "stricmp.h"
#include "IszImageStream.h"
#include "MemoryStateFile.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "xml/Node.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "AppConfig.h"
#include "PathUtils.h"
#include "Profiler.h"
#include "iop/IopBios.h"
#include "iop/DirectoryDevice.h"
#include "iop/IsoDevice.h"
#include "Log.h"
#include "placeholder_def.h"

#define LOG_NAME        ("ps2vm")

#ifdef DEBUGGER_INCLUDED
#define TAGS_PATH       ("./tags/")
#endif

#define STATE_EE        ("ee")
#define STATE_VU0       ("vu0")
#define STATE_VU1       ("vu1")
#define STATE_RAM       ("ram")
#define STATE_SPR       ("spr")
#define STATE_VUMEM0    ("vumem0")
#define STATE_MICROMEM0 ("micromem0")
#define STATE_VUMEM1    ("vumem1")
#define STATE_MICROMEM1 ("micromem1")

#define PREF_PS2_HOST_DIRECTORY_DEFAULT     ("vfs/host")
#define PREF_PS2_MC0_DIRECTORY_DEFAULT      ("vfs/mc0")
#define PREF_PS2_MC1_DIRECTORY_DEFAULT      ("vfs/mc1")
#define PREF_PS2_FRAMESKIP_DEFAULT          (0)

#define     CPU_FREQUENCY       (0x11940000)

#define		FRAME_TICKS		    (CPU_FREQUENCY / 60)
#define     ONSCREEN_TICKS      (FRAME_TICKS * 9 / 10)
#define     VBLANK_TICKS        (FRAME_TICKS / 10)

#define     SPU_UPDATE_TICKS    (1000)

#define		FAKE_IOP_RAM_SIZE	(0x1000)

using namespace Framework;
using namespace std;
using namespace std::tr1;
namespace filesystem = boost::filesystem;

CPS2VM::CPS2VM() :
m_ram(new uint8[PS2::EERAMSIZE]),
m_bios(new uint8[PS2::EEBIOSSIZE]),
m_spr(new uint8[PS2::SPRSIZE]),
m_fakeIopRam(new uint8[FAKE_IOP_RAM_SIZE]),
m_pVUMem0(new uint8[PS2::VUMEM0SIZE]),
m_pMicroMem0(new uint8[PS2::MICROMEM0SIZE]),
m_pVUMem1(new uint8[PS2::VUMEM1SIZE]),
m_pMicroMem1(new uint8[PS2::MICROMEM1SIZE]),
m_thread(NULL),
m_EE(MEMORYMAP_ENDIAN_LSBF, 0x00000000, 0x20000000),
m_VU0(MEMORYMAP_ENDIAN_LSBF, 0x00000000, 0x00008000),
m_VU1(MEMORYMAP_ENDIAN_LSBF, 0x00000000, 0x00008000),
m_executor(m_EE),
m_nStatus(PAUSED),
m_nEnd(false),
m_pGS(NULL),
m_pPad(NULL),
m_singleStepEe(false),
m_singleStepIop(false),
m_singleStepVu1(false),
m_nVBlankTicks(0),
m_nInVBlank(false),
m_spuUpdateTicks(SPU_UPDATE_TICKS),
m_pCDROM0(NULL),
m_dmac(m_ram, m_spr, m_EE),
m_gif(m_pGS, m_ram, m_spr),
m_sif(m_dmac, m_ram, m_iop.m_ram),
m_vif(m_gif, m_ram, m_spr, CVIF::VPUINIT(m_pMicroMem0, m_pVUMem0, &m_VU0), CVIF::VPUINIT(m_pMicroMem1, m_pVUMem1, &m_VU1)),
m_intc(m_dmac),
m_timer(m_intc),
m_COP_SCU(MIPS_REGSIZE_64),
m_COP_FPU(MIPS_REGSIZE_64),
m_COP_VU(MIPS_REGSIZE_64),
m_MAVU0(false),
m_MAVU1(true)
{
    const char* basicDirectorySettings[] =
    {
        PREF_PS2_HOST_DIRECTORY, PREF_PS2_HOST_DIRECTORY_DEFAULT,
        PREF_PS2_MC0_DIRECTORY, PREF_PS2_MC0_DIRECTORY_DEFAULT,
        PREF_PS2_MC1_DIRECTORY, PREF_PS2_MC1_DIRECTORY_DEFAULT,
        NULL, NULL
    };

    for(unsigned int i = 0; basicDirectorySettings[i] != NULL; i += 2)
    {
        const char* setting = basicDirectorySettings[i + 0];
        const char* path = basicDirectorySettings[i + 1];

        CConfig::PathType relativePath = CAppConfig::Utf8ToPath(path);
        CConfig::PathType absolutePath = CAppConfig::GetBasePath() / relativePath;
        PathUtils::EnsurePathExists(absolutePath);
        string absoluteStringPath = CAppConfig::PathToUtf8(absolutePath);
        CAppConfig::GetInstance().RegisterPreferenceString(setting, absoluteStringPath.c_str());
    }

//  CAppConfig::GetInstance().RegisterPreferenceString(PREF_PS2_HOST_DIRECTORY, PREF_PS2_HOST_DIRECTORY_DEFAULT);
//  CAppConfig::GetInstance().RegisterPreferenceString(PREF_PS2_MC0_DIRECTORY, PREF_PS2_MC0_DIRECTORY_DEFAULT);
//  CAppConfig::GetInstance().RegisterPreferenceString(PREF_PS2_MC1_DIRECTORY, PREF_PS2_MC1_DIRECTORY_DEFAULT);
 
    CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_PS2_FRAMESKIP, PREF_PS2_FRAMESKIP_DEFAULT);

    m_iopOsPtr = Iop::CSubSystem::BiosPtr(new CIopBios(PS2::IOP_CLOCK_FREQ, m_iop.m_cpu, m_iop.m_ram, PS2::IOP_RAM_SIZE));
    m_iopOs = static_cast<CIopBios*>(m_iopOsPtr.get());
    m_os = new CPS2OS(m_EE, m_ram, m_bios, m_pGS, m_sif, *m_iopOs);
}

CPS2VM::~CPS2VM()
{
    delete m_os;
    {
        //Big hack to force deletion of the IopBios
        m_iop.SetBios(Iop::CSubSystem::BiosPtr());
        m_iopOs = NULL;
        m_iopOsPtr.reset();
    }
}

//////////////////////////////////////////////////
//Various Message Functions
//////////////////////////////////////////////////

void CPS2VM::CreateGSHandler(const CGSHandler::FactoryFunction& factoryFunction)
{
	if(m_pGS != NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::CreateGsImpl, this, factoryFunction), true);
}

CGSHandler* CPS2VM::GetGSHandler()
{
	return m_pGS;
}

void CPS2VM::DestroyGSHandler()
{
	if(m_pGS == NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::DestroyGsImpl, this), true);
}

void CPS2VM::CreatePadHandler(const CPadHandler::FactoryFunction& factoryFunction)
{
    if(m_pPad != NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::CreatePadHandlerImpl, this, factoryFunction), true);
}

void CPS2VM::DestroyPadHandler()
{
	if(m_pPad == NULL) return;
    m_mailBox.SendCall(bind(&CPS2VM::DestroyPadHandlerImpl, this), true);
}

CVirtualMachine::STATUS CPS2VM::GetStatus() const
{
    return m_nStatus;
}

void CPS2VM::StepEe()
{
    if(GetStatus() == RUNNING) return;
    m_singleStepEe = true;
    m_mailBox.SendCall(bind(&CPS2VM::ResumeImpl, this), true);
}

void CPS2VM::StepIop()
{
    if(GetStatus() == RUNNING) return;
    m_singleStepIop = true;
    m_mailBox.SendCall(bind(&CPS2VM::ResumeImpl, this), true);
}

void CPS2VM::StepVu1()
{
    if(GetStatus() == RUNNING) return;
    m_singleStepVu1 = true;
    m_mailBox.SendCall(bind(&CPS2VM::ResumeImpl, this), true);
}

void CPS2VM::Resume()
{
    if(m_nStatus == RUNNING) return;
    m_mailBox.SendCall(bind(&CPS2VM::ResumeImpl, this), true);
    m_OnRunningStateChange();
}

void CPS2VM::Pause()
{
	if(m_nStatus == PAUSED) return;
    m_mailBox.SendCall(bind(&CPS2VM::PauseImpl, this), true);
    m_OnMachineStateChange();
    m_OnRunningStateChange();
}

void CPS2VM::Reset()
{
    assert(m_nStatus == PAUSED);
    ResetVM();
}

void CPS2VM::DumpEEThreadSchedule()
{
//	if(m_pOS == NULL) return;
	if(m_nStatus != PAUSED) return;
	m_os->DumpThreadSchedule();
}

void CPS2VM::DumpEEIntcHandlers()
{
//	if(m_pOS == NULL) return;
	if(m_nStatus != PAUSED) return;
	m_os->DumpIntcHandlers();
}

void CPS2VM::DumpEEDmacHandlers()
{
//	if(m_pOS == NULL) return;
	if(m_nStatus != PAUSED) return;
	m_os->DumpDmacHandlers();
}

void CPS2VM::Initialize()
{
	CreateVM();
	m_nEnd = false;
    m_thread = new boost::thread(bind(&CPS2VM::EmuThread, this));
}

void CPS2VM::Destroy()
{
    m_mailBox.SendCall(bind(&CPS2VM::DestroyImpl, this));
    if(m_thread)
    {
	    m_thread->join();
	    delete m_thread;
    }
	DestroyVM();
}

unsigned int CPS2VM::SaveState(const char* sPath)
{
    unsigned int result = 0;
    m_mailBox.SendCall(bind(&CPS2VM::SaveVMState, this, sPath, tr1::ref(result)), true);
    return result;
}

unsigned int CPS2VM::LoadState(const char* sPath)
{
    unsigned int result = 0;
    m_mailBox.SendCall(bind(&CPS2VM::LoadVMState, this, sPath, tr1::ref(result)), true);
    return result;
}

#ifdef DEBUGGER_INCLUDED

#define TAGS_SECTION_TAGS           ("tags")
#define TAGS_SECTION_EE_FUNCTIONS   ("ee_functions")
#define TAGS_SECTION_EE_COMMENTS    ("ee_comments")
#define TAGS_SECTION_VU1_FUNCTIONS  ("vu1_functions")
#define TAGS_SECTION_VU1_COMMENTS   ("vu1_comments")
#define TAGS_SECTION_IOP            ("iop")
#define TAGS_SECTION_IOP_FUNCTIONS  ("functions")
#define TAGS_SECTION_IOP_COMMENTS   ("comments")

string CPS2VM::MakeDebugTagsPackagePath(const char* packageName)
{
    filesystem::path tagsPath(TAGS_PATH);
    if(!filesystem::exists(tagsPath))
    {
        filesystem::create_directory(tagsPath);
    }
    return string(TAGS_PATH) + string(packageName) + string(".tags.xml");
}

void CPS2VM::LoadDebugTags(const char* packageName)
{
    try
    {
        string packagePath = MakeDebugTagsPackagePath(packageName);
        CStdStream stream(packagePath.c_str(), "rb");
        boost::scoped_ptr<Xml::CNode> document(Xml::CParser::ParseDocument(&stream));
        Xml::CNode* tagsNode = document->Select(TAGS_SECTION_TAGS);
        if(!tagsNode) return;
        m_EE.m_Functions.Unserialize(tagsNode, TAGS_SECTION_EE_FUNCTIONS);
        m_EE.m_Comments.Unserialize(tagsNode, TAGS_SECTION_EE_COMMENTS);
        m_VU1.m_Functions.Unserialize(tagsNode, TAGS_SECTION_VU1_FUNCTIONS);
        m_VU1.m_Comments.Unserialize(tagsNode, TAGS_SECTION_VU1_COMMENTS);
        {
            Xml::CNode* sectionNode = tagsNode->Select(TAGS_SECTION_IOP);
            if(sectionNode)
            {
                m_iop.m_cpu.m_Functions.Unserialize(sectionNode, TAGS_SECTION_IOP_FUNCTIONS);
                m_iop.m_cpu.m_Comments.Unserialize(sectionNode, TAGS_SECTION_IOP_COMMENTS);
                m_iopOs->LoadDebugTags(sectionNode);
            }
        }
    }
    catch(...)
    {

    }
}

void CPS2VM::SaveDebugTags(const char* packageName)
{
    try
    {
        string packagePath = MakeDebugTagsPackagePath(packageName);
        CStdStream stream(packagePath.c_str(), "wb");
        boost::scoped_ptr<Xml::CNode> document(new Xml::CNode(TAGS_SECTION_TAGS, true)); 
        m_EE.m_Functions.Serialize(document.get(), TAGS_SECTION_EE_FUNCTIONS);
        m_EE.m_Comments.Serialize(document.get(), TAGS_SECTION_EE_COMMENTS);
        m_VU1.m_Functions.Serialize(document.get(), TAGS_SECTION_VU1_FUNCTIONS);
        m_VU1.m_Comments.Serialize(document.get(), TAGS_SECTION_VU1_COMMENTS);
        {
            Xml::CNode* iopNode = new Xml::CNode(TAGS_SECTION_IOP, true);
            m_iop.m_cpu.m_Functions.Serialize(iopNode, TAGS_SECTION_IOP_FUNCTIONS);
            m_iop.m_cpu.m_Comments.Serialize(iopNode, TAGS_SECTION_IOP_COMMENTS);
            m_iopOs->SaveDebugTags(iopNode);
            document->InsertNode(iopNode);
        }
        Xml::CWriter::WriteDocument(&stream, document.get());
    }
    catch(...)
    {

    }
}

#endif

void CPS2VM::SetFrameSkip(unsigned int frameSkip)
{
    m_frameSkip = frameSkip;
    CAppConfig::GetInstance().SetPreferenceInteger(PREF_PS2_FRAMESKIP, m_frameSkip);
}

//////////////////////////////////////////////////
//Non extern callable methods
//////////////////////////////////////////////////

void CPS2VM::CreateVM()
{
    printf("PS2VM: Virtual Machine Memory Usage: RAM: %i MBs, BIOS: %i MBs, SPR: %i KBs.\r\n", 
        PS2::EERAMSIZE / 0x100000, PS2::EEBIOSSIZE / 0x100000, PS2::SPRSIZE / 0x1000);
	
	//EmotionEngine context setup
    {
        //Read map
	    m_EE.m_pMemoryMap->InsertReadMap(0x00000000, 0x01FFFFFF, m_ram,					                                        0x00);
	    m_EE.m_pMemoryMap->InsertReadMap(0x02000000, 0x02003FFF, m_spr,					                                        0x01);
        m_EE.m_pMemoryMap->InsertReadMap(0x10000000, 0x10FFFFFF, bind(&CPS2VM::IOPortReadHandler, this, PLACEHOLDER_1),         0x02);
        m_EE.m_pMemoryMap->InsertReadMap(0x12000000, 0x12FFFFFF, bind(&CPS2VM::IOPortReadHandler, this, PLACEHOLDER_1),         0x03);
		m_EE.m_pMemoryMap->InsertReadMap(0x1C000000, 0x1C001000, m_fakeIopRam,													0x04);
		m_EE.m_pMemoryMap->InsertReadMap(0x1FC00000, 0x1FFFFFFF, m_bios,				                                        0x05);

        //Write map
        m_EE.m_pMemoryMap->InsertWriteMap(0x00000000, 0x01FFFFFF, m_ram,				                                                    0x00);
	    m_EE.m_pMemoryMap->InsertWriteMap(0x02000000, 0x02003FFF, m_spr,				                                                    0x01);
        m_EE.m_pMemoryMap->InsertWriteMap(0x10000000, 0x10FFFFFF, bind(&CPS2VM::IOPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2),    0x02);
        m_EE.m_pMemoryMap->InsertWriteMap(0x12000000, 0x12FFFFFF, bind(&CPS2VM::IOPortWriteHandler,	this, PLACEHOLDER_1, PLACEHOLDER_2),    0x03);

        m_EE.m_pMemoryMap->SetWriteNotifyHandler(bind(&CPS2VM::EEMemWriteHandler, this, PLACEHOLDER_1));

	    //Instruction map
        m_EE.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x01FFFFFF, m_ram,			                        0x00);
	    m_EE.m_pMemoryMap->InsertInstructionMap(0x1FC00000, 0x1FFFFFFF, m_bios,				                    0x01);

	    m_EE.m_pArch			= &m_EEArch;
	    m_EE.m_pCOP[0]			= &m_COP_SCU;
	    m_EE.m_pCOP[1]			= &m_COP_FPU;
	    m_EE.m_pCOP[2]			= &m_COP_VU;

        m_EE.m_handlerParam     = this;
        m_EE.m_pAddrTranslator	= CPS2OS::TranslateAddress;
        m_EE.m_pSysCallHandler  = EESysCallHandlerStub;
#ifdef DEBUGGER_INCLUDED
        m_EE.m_pTickFunction	= EETickFunctionStub;
#else
	    m_EE.m_pTickFunction	= NULL;
#endif
    }

    //Vector Unit 0 context setup
    {
	    m_VU0.m_pMemoryMap->InsertReadMap(0x00000000, 0x00000FFF, m_pVUMem0,                                                    0x01);
	    m_VU0.m_pMemoryMap->InsertReadMap(0x00001000, 0x00001FFF, m_pVUMem0,                                                    0x02);
	    m_VU0.m_pMemoryMap->InsertReadMap(0x00002000, 0x00002FFF, m_pVUMem0,                                                    0x03);
	    m_VU0.m_pMemoryMap->InsertReadMap(0x00003000, 0x00003FFF, m_pVUMem0,                                                    0x04);
        m_VU0.m_pMemoryMap->InsertReadMap(0x00004000, 0x00008FFF, bind(&CPS2VM::Vu0IoPortReadHandler, this, PLACEHOLDER_1),     0x05);

	    m_VU0.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00000FFF, m_pVUMem0,                                                                 0x01);
	    m_VU0.m_pMemoryMap->InsertWriteMap(0x00001000, 0x00001FFF, m_pVUMem0,                                                                 0x02);
	    m_VU0.m_pMemoryMap->InsertWriteMap(0x00002000, 0x00002FFF, m_pVUMem0,                                                                 0x03);
	    m_VU0.m_pMemoryMap->InsertWriteMap(0x00003000, 0x00003FFF, m_pVUMem0,                                                                 0x04);
        m_VU0.m_pMemoryMap->InsertWriteMap(0x00004000, 0x00008FFF, bind(&CPS2VM::Vu0IoPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2),  0x05);

        m_VU0.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00000FFF, m_pMicroMem0,                                  0x00);

	    m_VU0.m_pArch			= &m_MAVU0;
	    m_VU0.m_pAddrTranslator	= CMIPS::TranslateAddress64;
        m_VU0.m_pTickFunction   = NULL;
    }

    //Vector Unit 1 context setup
    {
	    m_VU1.m_pMemoryMap->InsertReadMap(0x00000000, 0x00003FFF, m_pVUMem1,	                                                     0x00);
        m_VU1.m_pMemoryMap->InsertReadMap(0x00008000, 0x00008FFF, bind(&CPS2VM::Vu1IoPortReadHandler, this, PLACEHOLDER_1),          0x01);

	    m_VU1.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00003FFF, m_pVUMem1,	                                                              0x00);
        m_VU1.m_pMemoryMap->InsertWriteMap(0x00008000, 0x00008FFF, bind(&CPS2VM::Vu1IoPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2),  0x01);

	    m_VU1.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00003FFF, m_pMicroMem1,                                  0x01);

        m_VU1.m_pArch			= &m_MAVU1;
	    m_VU1.m_pAddrTranslator	= CMIPS::TranslateAddress64;

#ifdef DEBUGGER_INCLUDED
	    m_VU1.m_pTickFunction	= VU1TickFunctionStub;
#else
	    m_VU1.m_pTickFunction	= NULL;
#endif
    }

    m_dmac.SetChannelTransferFunction(0, bind(&CVIF::ReceiveDMA0, &m_vif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_4));
    m_dmac.SetChannelTransferFunction(1, bind(&CVIF::ReceiveDMA1, &m_vif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_4));
    m_dmac.SetChannelTransferFunction(2, bind(&CGIF::ReceiveDMA, &m_gif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
    m_dmac.SetChannelTransferFunction(4, bind(&CIPU::ReceiveDMA4, &m_ipu, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_4, m_ram));
    m_dmac.SetChannelTransferFunction(5, bind(&CSIF::ReceiveDMA5, &m_sif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
    m_dmac.SetChannelTransferFunction(6, bind(&CSIF::ReceiveDMA6, &m_sif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));

    m_ipu.SetDMA3ReceiveHandler(bind(&CDMAC::ResumeDMA3, &m_dmac, PLACEHOLDER_1, PLACEHOLDER_2));

	CDROM0_Initialize();

	ResetVM();

	printf("PS2VM: Created PS2 Virtual Machine.\r\n");
}

void CPS2VM::ResetVM()
{
    Pause();

    m_os->Release();
    m_executor.Clear();

    memset(m_ram,			0, PS2::EERAMSIZE);
    memset(m_spr,			0, PS2::SPRSIZE);
	memset(m_fakeIopRam,	0, FAKE_IOP_RAM_SIZE);
	memset(m_pVUMem0,       0, PS2::VUMEM0SIZE);
    memset(m_pMicroMem0,    0, PS2::MICROMEM0SIZE);
    memset(m_pVUMem1,		0, PS2::VUMEM1SIZE);
    memset(m_pMicroMem1,	0, PS2::MICROMEM1SIZE);

    m_iop.Reset();
    m_iop.SetBios(m_iopOsPtr);

	//LoadBIOS();

	//Reset Contexts
    m_EE.Reset();
    m_VU0.Reset();
    m_VU1.Reset();

	m_EE.m_Comments.RemoveTags();
	m_EE.m_Functions.RemoveTags();
    m_VU0.m_Comments.RemoveTags();
    m_VU0.m_Functions.RemoveTags();
    m_VU1.m_Comments.RemoveTags();
    m_VU1.m_Functions.RemoveTags();

	//Reset subunits
    m_sif.Reset();
    m_ipu.Reset();
    m_gif.Reset();
    m_vif.Reset();
    m_dmac.Reset();
	m_intc.Reset();
    m_timer.Reset();

	if(m_pGS != NULL)
	{
		m_pGS->Reset();
	}

    m_os->Initialize();
    m_iopOs->Reset(new Iop::CSifManPs2(m_sif, m_ram, m_iop.m_ram));

	CDROM0_Reset();

    m_iopOs->GetIoman()->RegisterDevice("host", Iop::CIoman::DevicePtr(new Iop::Ioman::CDirectoryDevice(PREF_PS2_HOST_DIRECTORY)));
    m_iopOs->GetIoman()->RegisterDevice("mc0", Iop::CIoman::DevicePtr(new Iop::Ioman::CDirectoryDevice(PREF_PS2_MC0_DIRECTORY)));
    m_iopOs->GetIoman()->RegisterDevice("mc1", Iop::CIoman::DevicePtr(new Iop::Ioman::CDirectoryDevice(PREF_PS2_MC1_DIRECTORY)));
    m_iopOs->GetIoman()->RegisterDevice("cdrom0", Iop::CIoman::DevicePtr(new Iop::Ioman::CIsoDevice(m_pCDROM0)));

    m_frameSkip = CAppConfig::GetInstance().GetPreferenceInteger(PREF_PS2_FRAMESKIP);

    m_nVBlankTicks = ONSCREEN_TICKS;
    m_nInVBlank = false;

    RegisterModulesInPadHandler();
	FillFakeIopRam();
}

void CPS2VM::DestroyVM()
{
	CDROM0_Destroy();

	FREEPTR(m_ram);
	FREEPTR(m_bios);
	FREEPTR(m_fakeIopRam);
}

void CPS2VM::SaveVMState(const char* sPath, unsigned int& result)
{
	if(m_pGS == NULL)
	{
		printf("PS2VM: GS Handler was not instancied. Cannot save state.\r\n");
        result = 1;
        return;
	}
    
    try
    {
        CStdStream stateStream(sPath, "wb");
        CZipArchiveWriter archive;

        archive.InsertFile(new CMemoryStateFile(STATE_EE,           &m_EE.m_State,  sizeof(MIPSSTATE)));
        archive.InsertFile(new CMemoryStateFile(STATE_VU0,          &m_VU0.m_State, sizeof(MIPSSTATE)));
        archive.InsertFile(new CMemoryStateFile(STATE_VU1,          &m_VU1.m_State, sizeof(MIPSSTATE)));
        archive.InsertFile(new CMemoryStateFile(STATE_RAM,          m_ram,			PS2::EERAMSIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_SPR,          m_spr,			PS2::SPRSIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_VUMEM0,       m_pVUMem0,      PS2::VUMEM0SIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_MICROMEM0,    m_pMicroMem0,   PS2::MICROMEM0SIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_VUMEM1,       m_pVUMem1,      PS2::VUMEM1SIZE));
        archive.InsertFile(new CMemoryStateFile(STATE_MICROMEM1,    m_pMicroMem1,   PS2::MICROMEM1SIZE));

        m_iop.SaveState(archive);
        m_pGS->SaveState(archive);
        m_dmac.SaveState(archive);
        m_intc.SaveState(archive);
        m_sif.SaveState(archive);
        m_vif.SaveState(archive);
        m_iopOs->GetDbcman()->SaveState(archive);
        m_iopOs->GetPadman()->SaveState(archive);
        //TODO: Save CDVDFSV state

        archive.Write(stateStream);
    }
    catch(...)
    {
        result = 1;
        return;
    }

    printf("PS2VM: Saved state to file '%s'.\r\n", sPath);

    result = 0;
}

void CPS2VM::LoadVMState(const char* sPath, unsigned int& result)
{
	if(m_pGS == NULL)
	{
		printf("PS2VM: GS Handler was not instancied. Cannot load state.\r\n");
        result = 1;
        return;
	}

    try
    {
        CStdStream stateStream(sPath, "rb");
		CZipArchiveReader archive(stateStream);
		
		try
		{
			archive.BeginReadFile(STATE_EE          )->Read(&m_EE.m_State,  sizeof(MIPSSTATE));
            archive.BeginReadFile(STATE_VU0         )->Read(&m_VU0.m_State, sizeof(MIPSSTATE));
			archive.BeginReadFile(STATE_VU1         )->Read(&m_VU1.m_State, sizeof(MIPSSTATE));
			archive.BeginReadFile(STATE_RAM         )->Read(m_ram,			PS2::EERAMSIZE);
			archive.BeginReadFile(STATE_SPR         )->Read(m_spr,			PS2::SPRSIZE);
            archive.BeginReadFile(STATE_VUMEM0      )->Read(m_pVUMem0,      PS2::VUMEM0SIZE);
            archive.BeginReadFile(STATE_MICROMEM0   )->Read(m_pMicroMem0,   PS2::MICROMEM0SIZE);
			archive.BeginReadFile(STATE_VUMEM1      )->Read(m_pVUMem1,      PS2::VUMEM1SIZE);
			archive.BeginReadFile(STATE_MICROMEM1   )->Read(m_pMicroMem1,   PS2::MICROMEM1SIZE);

            m_iop.LoadState(archive);
			m_pGS->LoadState(archive);
			m_dmac.LoadState(archive);
			m_intc.LoadState(archive);
			m_sif.LoadState(archive);
			m_vif.LoadState(archive);
			m_iopOs->GetDbcman()->LoadState(archive);
			m_iopOs->GetPadman()->LoadState(archive);
		}
		catch(...)
		{
			//Any error that occurs in the previous block is critical
			PauseImpl();
			throw;
		}
    }
    catch(...)
    {
        result = 1;
        return;
    }

    printf("PS2VM: Loaded state from file '%s'.\r\n", sPath);

    m_executor.Clear();
	m_OnMachineStateChange();

    result = 0;
}

void CPS2VM::PauseImpl()
{
    m_nStatus = PAUSED;
//    printf("PS2VM: Virtual Machine paused.\r\n");
}

void CPS2VM::ResumeImpl()
{
    m_nStatus = RUNNING;
//    printf("PS2VM: Virtual Machine started.\r\n");
}

void CPS2VM::DestroyImpl()
{
    DELETEPTR(m_pGS);
    m_nEnd = true;
}

void CPS2VM::CreateGsImpl(const CGSHandler::FactoryFunction& factoryFunction)
{
    m_pGS = factoryFunction();
    m_pGS->Initialize();
    m_pGS->OnNewFrame.connect(bind(&CPS2VM::OnGsNewFrame, this));
}

void CPS2VM::DestroyGsImpl()
{
    m_pGS->Release();
    DELETEPTR(m_pGS);
}

void CPS2VM::CreatePadHandlerImpl(const CPadHandler::FactoryFunction& factoryFunction)
{
    m_pPad = factoryFunction();
    RegisterModulesInPadHandler();
}

void CPS2VM::DestroyPadHandlerImpl()
{
    DELETEPTR(m_pPad);
}

void CPS2VM::OnGsNewFrame()
{
    m_frameNumber++;
    bool drawFrame = (m_frameSkip == 0) ? true : (m_frameNumber % (m_frameSkip + 1)) == 0;
    if(m_pGS != NULL)
    {
        m_pGS->SetEnabled(drawFrame);
    }
    m_vif.SetEnabled(drawFrame);
}

void CPS2VM::UpdateSpu()
{
    Iop::CSpuBase* spu[2] = { &m_iop.m_spuCore0, &m_iop.m_spuCore1 };
    const int sampleRate = 44100;
    const int sampleCount = 352;
	size_t bufferSize = sampleCount * sizeof(int16);

	for(unsigned int i = 0; i < 2; i++)
	{
		if(spu[i]->IsEnabled())
		{
			int16* tempSamples = reinterpret_cast<int16*>(alloca(bufferSize));
			spu[i]->Render(tempSamples, sampleCount, sampleRate);

			//for(unsigned int j = 0; j < sampleCount; j++)
			//{
			//	int32 resultSample = static_cast<int32>(samples[j]) + static_cast<int32>(tempSamples[j]);
			//	resultSample = max<int32>(resultSample, SHRT_MIN);
			//	resultSample = min<int32>(resultSample, SHRT_MAX);
			//	samples[j] = static_cast<int16>(resultSample);
			//}
		}
	}
}

void CPS2VM::CDROM0_Initialize()
{
	CAppConfig::GetInstance().RegisterPreferenceString(PS2VM_CDROM0PATH, "");
	m_pCDROM0 = NULL;
}

void CPS2VM::CDROM0_Reset()
{
	DELETEPTR(m_pCDROM0);
	CDROM0_Mount(CAppConfig::GetInstance().GetPreferenceString(PS2VM_CDROM0PATH));
}

void CPS2VM::CDROM0_Mount(const char* sPath)
{
	//Check if there's an m_pCDROM0 already
	//Check if files are linked to this m_pCDROM0 too and do something with them

    size_t pathLength = strlen(sPath);
	if(pathLength != 0)
	{
		try
		{
	        CStream* pStream(NULL);
            const char* extension = "";
            if(pathLength >= 4)
            {
                extension = sPath + pathLength - 4;
            }

			//Gotta think of something better than that...
            if(!stricmp(extension, ".isz"))
            {
                pStream = new CIszImageStream(new CStdStream(sPath, "rb"));
            }
#ifdef WIN32
            else if(sPath[0] == '\\')
            {
	            pStream = new Win32::CVolumeStream(sPath[4]);
            }
#else
            else
            {
			    pStream = new Posix::CVolumeStream(sPath);
            }
#endif

            //If it's null after all that, just feed it to a StdStream
            if(pStream == NULL)
            {
                pStream = new CStdStream(sPath, "rb");
            }

			m_pCDROM0 = new CISO9660(pStream);
            SetIopCdImage(m_pCDROM0);
		}
		catch(const exception& Exception)
		{
			printf("PS2VM: Error mounting cdrom0 device: %s\r\n", Exception.what());
		}
	}

	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, sPath);
}

void CPS2VM::CDROM0_Destroy()
{
    SetIopCdImage(NULL);
    DELETEPTR(m_pCDROM0);
}

void CPS2VM::SetIopCdImage(CISO9660* image)
{
    m_iopOs->GetCdvdfsv()->SetIsoImage(image);
    m_iopOs->GetCdvdman()->SetIsoImage(image);
}

void CPS2VM::LoadBIOS()
{
    CStdStream BiosStream(fopen("./vfs/rom0/scph10000.bin", "rb"));
    BiosStream.Read(m_bios, PS2::EEBIOSSIZE);
}

void CPS2VM::RegisterModulesInPadHandler()
{
	if(m_pPad == NULL) return;

	m_pPad->RemoveAllListeners();
	m_pPad->InsertListener(m_iopOs->GetDbcman());
    m_pPad->InsertListener(m_iopOs->GetPadman());
}

void CPS2VM::FillFakeIopRam()
{
	struct IOPMODINFO
	{
		uint32 nextPtr;
		uint32 namePtr;

		uint16 version;
		uint16 newFlags;
		uint16 id;
		uint16 flags;

		uint32 entry;
		uint32 gp;
		uint32 textStart;
		uint32 textSize;
		uint32 dataSize;
		uint32 bssSize;
		uint32 unused1;
		uint32 unused2;
	};

	IOPMODINFO* moduleInfo = reinterpret_cast<IOPMODINFO*>(m_fakeIopRam + 0x800);
	moduleInfo->nextPtr = 0x0;
	moduleInfo->namePtr = 0xC00;

	strcpy(reinterpret_cast<char*>(m_fakeIopRam + 0xC00), "sio2man");
}

uint32 CPS2VM::IOPortReadHandler(uint32 nAddress)
{
#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	uint32 nReturn = 0;
	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
		nReturn = m_timer.GetRegister(nAddress);
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
        nReturn = m_ipu.GetRegister(nAddress);
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		nReturn = m_dmac.GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		nReturn = m_intc.GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		nReturn = m_dmac.GetRegister(nAddress);
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_pGS != NULL)
		{
			nReturn = m_pGS->ReadPrivRegister(nAddress);		
		}
	}
	else
	{
		printf("PS2VM: Read an unhandled IO port (0x%0.8X).\r\n", nAddress);
	}

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif

	return nReturn;
}

uint32 CPS2VM::IOPortWriteHandler(uint32 nAddress, uint32 nData)
{
#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
		m_timer.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
        m_ipu.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10007000 && nAddress <= 0x1000702F)
	{
        m_ipu.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		m_dmac.SetRegister(nAddress, nData);
    }
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		m_intc.SetRegister(nAddress, nData);
	}
	else if(nAddress == 0x1000F180)
	{
		//stdout data
        throw runtime_error("Not implemented.");
//        m_sif.GetFileIO()->Write(1, 1, &nData);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		m_dmac.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_pGS != NULL)
		{
			m_pGS->WritePrivRegister(nAddress, nData);
		}
	}
	else
	{
		printf("PS2VM: Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, nData, m_EE.m_State.nPC);
	}

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_EEZONE);
#endif

    return 0;
}

uint32 CPS2VM::Vu0IoPortReadHandler(uint32 address)
{
    uint32 result = 0;
    switch(address)
    {
    case CVIF::VU_ITOP:
        result = m_vif.GetITop0();
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Read an unhandled VU0 IO port (0x%0.8X).\r\n", address);
        break;
    }
    return result;
}

uint32 CPS2VM::Vu0IoPortWriteHandler(uint32 address, uint32 value)
{
    switch(address)
    {
    default:
        CLog::GetInstance().Print(LOG_NAME, "Wrote an unhandled VU0 IO port (0x%0.8X, 0x%0.8X).\r\n", 
            address, value);
        break;
    }
    return 0;
}

uint32 CPS2VM::Vu1IoPortReadHandler(uint32 address)
{
    uint32 result = 0xCCCCCCCC;
    switch(address)
    {
    case CVIF::VU_ITOP:
        result = m_vif.GetITop1();
        break;
    case CVIF::VU_TOP:
        result = m_vif.GetTop1();
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Read an unhandled VU1 IO port (0x%0.8X).\r\n", address);
        break;
    }
    return result;
}

uint32 CPS2VM::Vu1IoPortWriteHandler(uint32 address, uint32 value)
{
    switch(address)
    {
    case CVIF::VU_XGKICK:
        m_vif.ProcessXGKICK(value);
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Wrote an unhandled VU1 IO port (0x%0.8X, 0x%0.8X).\r\n", 
            address, value);
        break;
    }
    return 0;
}

void CPS2VM::EEMemWriteHandler(uint32 nAddress)
{
    if(nAddress < PS2::EERAMSIZE)
	{
		//Check if the block we're about to invalidate is the same
		//as the one we're executing in

//		CCacheBlock* pBlock;
//		pBlock = m_EE.m_pExecMap->FindBlock(nAddress);
//		if(m_EE.m_pExecMap->FindBlock(m_EE.m_State.nPC) != pBlock)
//		{
//			m_EE.m_pExecMap->InvalidateBlock(nAddress);
//		}
//		else
//		{
#ifdef _DEBUG
//			printf("PS2VM: Warning. Writing to the same cache block as the one we're currently executing in. PC: 0x%0.8X\r\n",
//				m_EE.m_State.nPC);
#endif
//		}
	}
}

void CPS2VM::EESysCallHandlerStub(CMIPS* context)
{
//    CPS2VM& vm = *reinterpret_cast<CPS2VM*>(context->m_handlerParam);
//    vm.m_os.SysCallHandler();
}

unsigned int CPS2VM::EETickFunctionStub(unsigned int ticks, CMIPS* context)
{
    return reinterpret_cast<CPS2VM*>(context->m_handlerParam)->EETickFunction(ticks);
}

unsigned int CPS2VM::VU1TickFunctionStub(unsigned int ticks, CMIPS* context)
{
    return reinterpret_cast<CPS2VM*>(context->m_handlerParam)->VU1TickFunction(ticks);
}

unsigned int CPS2VM::EETickFunction(unsigned int nTicks)
{

#ifdef DEBUGGER_INCLUDED

	if(m_EE.m_nIllOpcode != MIPS_INVALID_PC)
	{
		printf("PS2VM: (EmotionEngine) Illegal opcode encountered at 0x%0.8X.\r\n", m_EE.m_nIllOpcode);
		m_EE.m_nIllOpcode = MIPS_INVALID_PC;
		assert(0);
	}

	if(m_EE.MustBreak())
	{
		return 1;
	}

	//TODO: Check if we can remove this if there's no debugger around
//	if(m_MsgBox.IsMessagePending())
    if(m_mailBox.IsPending())
	{
		return 1;
	}

#endif

	return 0;
}

unsigned int CPS2VM::VU1TickFunction(unsigned int nTicks)
{
#ifdef DEBUGGER_INCLUDED

	if(m_VU1.m_nIllOpcode != MIPS_INVALID_PC)
	{
		printf("PS2VM: (VectorUnit1) Illegal/unimplemented instruction encountered at 0x%0.8X.\r\n", m_VU1.m_nIllOpcode);
		m_VU1.m_nIllOpcode = MIPS_INVALID_PC;
	}

	if(m_VU1.MustBreak())
	{
		return 1;
	}

	//TODO: Check if we can remove this if there's no debugger around
//	if(m_MsgBox.IsMessagePending())
    if(m_mailBox.IsPending())
    {
		return 1;
	}

#endif

	return 0;
}

void CPS2VM::EmuThread()
{
    //BEGIN: Frame limiter init
//    unsigned int lastFrameCount = 0;
//    xtime lastFrameTime;
//    xtime_get(&lastFrameTime, boost::TIME_UTC);
    //END
	while(1)
	{
        while(m_mailBox.IsPending())
        {
            m_mailBox.ReceiveCall();
        }
		if(m_nEnd) break;
		if(m_nStatus == PAUSED)
		{
            //Sleep during 100ms
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 100 * 1000000;
            boost::thread::sleep(xt);
		}
		if(m_nStatus == RUNNING)
        {
            //Synchonization methods
            //1984.elf - CSR = CSR & 0x08; while(CSR & 0x08 == 0);
			if(m_nVBlankTicks <= 0)
			{
				m_nInVBlank = !m_nInVBlank;
				if(m_nInVBlank)
				{
					m_nVBlankTicks += VBLANK_TICKS;
                    m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_START);

                    if(m_pGS != NULL)
                    {
                        m_pGS->ForcedFlip();
					    m_pGS->SetVBlank();
                    }

                    if(m_pPad != NULL)
                    {
                        m_pPad->Update(m_ram);
                    }
				}
				else
				{
					m_nVBlankTicks += ONSCREEN_TICKS;
                    m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_END);
					if(m_pGS != NULL)
					{
						m_pGS->ResetVBlank();
					}
				}
			}

            //if(m_spuUpdateTicks <= 0)
            //{
            //    UpdateSpu();
            //    m_spuUpdateTicks += SPU_UPDATE_TICKS;
            //}

            //EE execution
            {
                if(!m_vif.IsVu0Running() || (m_vif.IsVu0Running() && !m_vif.IsVu0WaitingForProgramEnd()))
                {
                    m_dmac.ResumeDMA0();
                }
                if(!m_vif.IsVu1Running() || (m_vif.IsVu1Running() && !m_vif.IsVu1WaitingForProgramEnd()))
                {
                    m_dmac.ResumeDMA1();
                }
                m_dmac.ResumeDMA4();
                if(!m_EE.m_State.nHasException)
                {
                    if((m_EE.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL) == 0)
                    {
                        m_sif.ProcessPackets();
                    }
                }
                if(!m_EE.m_State.nHasException)
                {
                    if(m_intc.IsInterruptPending())
                    {
                        m_os->ExceptionHandler();
                        //Do we need to do some special treatment when we're serving an interrupt?
                        //m_EE.m_State.nHasException = 1;
                    }
                }
                if(!m_EE.m_State.nHasException)
                {
                    int executeQuota = m_singleStepEe ? 1 : 5000;
                    int executed = (executeQuota - m_executor.Execute(executeQuota));
                    m_EE.m_State.nCOP0[CCOP_SCU::COUNT] += (executed * 100);
				    m_nVBlankTicks -= executed;
                    m_spuUpdateTicks -= executed;
					m_timer.Count(executed);
                }
                if(m_EE.m_State.nHasException)
                {
                    m_os->SysCallHandler();
                    assert(!m_EE.m_State.nHasException);
                }
                {
                    CBasicBlock* nextBlock = m_executor.FindBlockAt(m_EE.m_State.nPC);
                    const int skipAmount = 50000;
                    if(nextBlock != NULL && nextBlock->GetSelfLoopCount() > 5000)
                    {
                        m_nVBlankTicks -= skipAmount;
                    }
                    else if(m_EE.m_State.nPC >= 0x1FC03100 && m_EE.m_State.nPC <= 0x1FC03110)
                    {
                        m_nVBlankTicks = 0;
                    }
                    /*else if(m_pGS != NULL && m_pGS->IsRenderDone())
                    {
                        m_nVBlankTicks -= skipAmount;
                        m_nVBlankTicks = 0;
                    }*/
                }
                //Castlevania speed hack
                {
    //                if(m_ram[0x00] == 0x01)
    //                {
    //                    m_nVBlankTicks = 0;
    //                    thread::yield();
    //                }
                }
                //BEGIN: Frame limiter
    //            if(m_pGS != NULL && lastFrameCount != m_pGS->GetFrameCount())
    //            {
    //                xtime currentTime;
    //                xtime_get(&currentTime, boost::TIME_UTC);
    //                xtime::xtime_nsec_t timeDiff = currentTime.nsec - lastFrameTime.nsec;
    //                if((currentTime.sec == lastFrameTime.sec) && (timeDiff < (1000000 * 8)))
    //                {
    //                    currentTime.nsec += (1000000 * 8) - timeDiff;
    //                    thread::sleep(currentTime);
    //                }
    //                lastFrameCount = m_pGS->GetFrameCount();
    //                xtime_get(&lastFrameTime, boost::TIME_UTC);
    //            }
                //END

                //IOP Execution
                m_iop.ExecuteCpu(m_singleStepIop);
				m_vif.ExecuteVu1(m_singleStepVu1);
            }
#ifdef DEBUGGER_INCLUDED
            if(
                m_executor.MustBreak() || 
                m_iop.m_executor.MustBreak() ||
				m_vif.MustVu1Break() ||
                m_singleStepEe || m_singleStepIop || m_singleStepVu1)
            {
                m_nStatus = PAUSED;
                m_singleStepEe = false;
                m_singleStepIop = false;
				m_singleStepVu1 = false;
                m_OnRunningStateChange();
                m_OnMachineStateChange();
            }
#endif
		}
	}
}
