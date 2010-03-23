#include "StdStream.h"
#include "PspBios.h"
#include "ElfFile.h"
#include "lexical_cast_ex.h"
#include "Log.h"
#include "COP_SCU.h"
#include "Psp_ThreadManForUser.h"
#include "Psp_StdioForUser.h"
#include "Psp_SysMemUserForUser.h"
#include "Psp_KernelLibrary.h"
#include "Psp_SasCore.h"

using namespace Psp;

#define LOGNAME				("PspBios")

#define RELOC_SECTION_ID	(0x700000A0)

#define BIOS_THREAD_LINK_HEAD_BASE		(Psp::CBios::CONTROL_BLOCK_START + 0x0000)
#define BIOS_CURRENT_THREAD_ID_BASE     (Psp::CBios::CONTROL_BLOCK_START + 0x0008)
#define BIOS_CURRENT_TIME_BASE          (Psp::CBios::CONTROL_BLOCK_START + 0x0010)
#define BIOS_HANDLERS_BASE              (Psp::CBios::CONTROL_BLOCK_START + 0x0080)
#define BIOS_HANDLERS_END				(BIOS_HEAPBLOCK_BASE - 1)
#define BIOS_HEAPBLOCK_BASE				(Psp::CBios::CONTROL_BLOCK_START + 0x0100)
#define BIOS_HEAPBLOCK_SIZE				(sizeof(Psp::CBios::HEAPBLOCK) * Psp::CBios::MAX_HEAPBLOCKS)
#define BIOS_MODULE_TRAMPOLINE_BASE		(BIOS_HEAPBLOCK_BASE + BIOS_HEAPBLOCK_SIZE)
#define BIOS_MODULE_TRAMPOLINE_END		(sizeof(Psp::CBios::MODULETRAMPOLINE) * Psp::CBios::MAX_MODULETRAMPOLINES)
#define BIOS_THREAD_BASE				(BIOS_MODULE_TRAMPOLINE_BASE + BIOS_MODULE_TRAMPOLINE_END)
#define BIOS_THREAD_END					(sizeof(Psp::CBios::THREAD) * Psp::CBios::MAX_THREADS)
#define BIOS_CALCULATED_END				(BIOS_THREAD_BASE + BIOS_THREAD_END)

CBios::MODULEFUNCTION CBios::g_IoFileMgrForUserFunctions[] =
{
	{	0x27EB27B8,		"sceIoLseek"		},
	{	0x109F50BC,		"sceIoOpen"			},
	{	0x42EC03AC,		"sceIoWrite"		},
	{	0x6A638D83,		"sceIoRead"			},
	{	0x810C4BC3,		"sceIoClose"		},
	{	0xA0B5A7C2,		"sceIoReadAsync"	},
	{	NULL,			NULL				},
};

CBios::MODULEFUNCTION CBios::g_SysMemUserForUserFunctions[] =
{
	{	0x91DE343C,		"sceKernelSetCompiledSdkVersion"	},
	{	0xF77D77CB,		"sceKernelSetCompilerVersion"		},
	{	0x237DBD4F,		"sceKernelAllocPartitionMemory"		},
	{	0x9D9A5BA1,		"sceKernelGetBlockHeadAddr"			},
	{	0xFE707FDF,		"sceKernelAllocMemoryBlock"			},
	{	0xDB83A952,		"sceKernelGetMemoryBlockAddr"		},
	{	0x50F61D8A,		"sceKernelFreeMemoryBlock"			},
};

CBios::MODULEFUNCTION CBios::g_ThreadManForUserFunctions[] =
{
	{	0x446D8DE6,		"sceKernelCreateThread"		},
	{	0xF475845D,		"sceKernelStartThread"		},
	{	0x278C0DF5,		"sceKernelWaitThreadEnd"	},
	{	0x9FA03CD3,		"sceKernelDeleteThread"		},
	{	0xCEADEB47,		"sceKernelDelayThread"		},
	{	0xAA73C935,		"sceKernelExitThread"		},
	{	0xE81CAF8F,		"sceKernelCreateCallback"	},
	{	0xD6DA4BA1,		"sceKernelCreateSema"		},
	{	0x4E3A1105,		"sceKernelWaitSema"			},
	{	0x3F53E640,		"sceKernelSignalSema"		},
	{	0x28B6489C,		"sceKernelDeleteSema"		},
	{	NULL,			NULL						},
};

CBios::MODULEFUNCTION CBios::g_StdioForUserFunctions[] =
{
	{	0xA6BAB2E9,		"sceKernelStdout"					},
	{	NULL,			NULL								},
};

CBios::MODULEFUNCTION CBios::g_LoadExecForUserFunctions[] =
{
	{	0x4AC57943,		"sceKernelRegisterExitCallback"		},
	{	NULL,			NULL								},
};

CBios::MODULEFUNCTION CBios::g_UtilsForUserFunctions[] =
{
	{	0xB435DEC5,		"sceKernelDcacheWritebackInvalidateAll"		},
	{	NULL,			NULL										},
};

CBios::MODULEFUNCTION CBios::g_SasCoreFunctions[] =
{
	{	0x76F01ACA,		"sasSetKeyOn"			},
	{	0xA0CF2FA4,		"sasSetKeyOff"			},
	{	0x019B25EB,		"sasSetADSR"			},
	{	0xCBCD4F79,		"sasSetSimpleADSR"		},
	{	0xAD84D37F,		"sasSetPitch"			},
	{	0x99944089,		"sasSetVoice"			},
	{	0xE1CD9561,		"sasSetVoicePCM"		},
	{	0x440CA7D8,		"sasSetVolume"			},
	{	0x2C8E6AB3,		"sasGetPauseFlag"		},
	{	0x68A46B95,		"sasGetEndFlag"			},
	{	0x07F58C24,		"sasGetAllEnvelope"		},
	{	0xA3589D81,		"sasCore"				},
	{	0x42778A9F,		"sasInit"				},
	{	NULL,			NULL					},
};

CBios::MODULEFUNCTION CBios::g_WaveFunctions[] =
{
	{	0xE2D56B2D,		"sceAudioOutputPanned"			},
	{	0x13F592BC,		"sceAudioOutputPannedBlocking"	},
	{	0xB011922F,		"WaveAudioGetRestSample"		},
	{	0x5EC81C55,		"sceAudioChReserve"				},
	{	0xCB2E439E,		"sceAudioSetChannelDataLen"		},
	{	NULL,			NULL							},
};

CBios::MODULEFUNCTION CBios::g_UmdUserFunctions[] =
{
	{	0xC6183D47,		"sceUmdActivate"			},
	{	0x6B4A146C,		"sceUmdGetDriveStat"		},
	{	0x8EF08FCE,		"sceUmdWaitDriveStat"		},
	{	0x6AF9B50A,		"sceUmdCancelWaitDriveStat"	},
	{	NULL,			NULL						},
};

CBios::MODULEFUNCTION CBios::g_UtilityFunctions[] =
{
	{	0x2A2B3DE0,		"sceUtilityLoadModule"		},
	{	0xE49BFE92,		"sceUtilityUnloadModule"	},
	{	NULL,			NULL						},
};

CBios::MODULEFUNCTION CBios::g_KernelLibraryFunctions[] =
{
	{	0x092968F4,		"sceKernelCpuSuspendIntr"	},
	{	0x5F10D406,		"sceKernelCpuResumeIntr"	},
};

CBios::MODULEFUNCTION CBios::g_ModuleMgrForUserFunctions[] =
{
	{	0xD8B73127,		"sceKernelGetModuleIdByAddress"	},
};

CBios::MODULE CBios::g_modules[] =
{
	{	"IoFileMgrForUser",		g_IoFileMgrForUserFunctions		},
	{	"SysMemUserForUser",	g_SysMemUserForUserFunctions	},
	{	"ThreadManForUser",		g_ThreadManForUserFunctions		},
	{	"LoadExecForUser",		g_LoadExecForUserFunctions		},
	{	"UtilsForUser",			g_UtilsForUserFunctions			},
	{	"ModuleMgrForUser",		g_ModuleMgrForUserFunctions		},
	{	"StdioForUser",			g_StdioForUserFunctions			},
	{	"sceSasCore",			g_SasCoreFunctions				},
	{	"sceAudio",				g_WaveFunctions					},
	{	"sceUmdUser",			g_UmdUserFunctions				},
	{	"sceUtility",			g_UtilityFunctions				},
	{	"Kernel_Library",		g_KernelLibraryFunctions		},
	{	NULL,					NULL							},
};

CBios::CBios(CMIPS& cpu, uint8* ram, uint32 ramSize)
: m_module(NULL)
, m_ram(ram)
, m_ramSize(ramSize)
, m_cpu(cpu)
, m_heapBlocks(reinterpret_cast<HEAPBLOCK*>(&ram[BIOS_HEAPBLOCK_BASE]), 1, MAX_HEAPBLOCKS)
, m_moduleTrampolines(reinterpret_cast<MODULETRAMPOLINE*>(&ram[BIOS_MODULE_TRAMPOLINE_BASE]), 1, MAX_MODULETRAMPOLINES)
, m_threads(reinterpret_cast<THREAD*>(&ram[BIOS_THREAD_BASE]), 1, MAX_THREADS)
, m_rescheduleNeeded(false)
, m_threadFinishAddress(0)
, m_idleFunctionAddress(0)
{
	BOOST_STATIC_ASSERT(BIOS_CALCULATED_END <= CBios::CONTROL_BLOCK_END);
	Reset();
}

CBios::~CBios()
{
	if(m_module != NULL)
	{
		delete m_module;
		m_module = NULL;
	}
}

void CBios::Reset()
{
	//Assemble handlers
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[BIOS_HANDLERS_BASE]));
		m_threadFinishAddress = AssembleThreadFinish(assembler);
//		m_returnFromExceptionAddress = AssembleReturnFromException(assembler);
		m_idleFunctionAddress = AssembleIdleFunction(assembler);
		assert(BIOS_HANDLERS_END > ((assembler.GetProgramSize() * 4) + BIOS_HANDLERS_BASE));
	}

	if(m_module != NULL)
	{
		delete m_module;
		m_module = NULL;
	}

	Heap_Init();

	m_moduleTrampolines.FreeAll();
	m_threads.FreeAll();

    ThreadLinkHead() = 0;
    CurrentThreadId() = -1;

	//Initialize modules
	m_modules.clear();

	m_ioFileMgrForUserModule = IoFileMgrForUserModulePtr(new CIoFileMgrForUser(m_ram)); 

	InsertModule(ModulePtr(new CThreadManForUser(*this, m_ram)));
	InsertModule(m_ioFileMgrForUserModule);
	InsertModule(ModulePtr(new CStdioForUser()));
	InsertModule(ModulePtr(new CSysMemUserForUser(*this, m_ram)));
	InsertModule(ModulePtr(new CKernelLibrary()));
	InsertModule(ModulePtr(new CSasCore()));

	//Initialize Io devices
	m_psfDevice = PsfDevicePtr(new CPsfDevice());

	m_ioFileMgrForUserModule->RegisterDevice("host0", m_psfDevice);
}

uint32 CBios::AssembleThreadFinish(CMIPSAssembler& assembler)
{
    uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0667);
    assembler.SYSCALL();
    return address;
}

uint32 CBios::AssembleReturnFromException(CMIPSAssembler& assembler)
{
    uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0668);
    assembler.SYSCALL();
    return address;
}

uint32 CBios::AssembleIdleFunction(CMIPSAssembler& assembler)
{
    uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0669);
    assembler.SYSCALL();
    return address;
}

void CBios::InsertModule(const ModulePtr& module)
{
	m_modules[module->GetName()] = module;
}

CPsfDevice* CBios::GetPsfDevice()
{
	return m_psfDevice.get();
}

void CBios::HandleLinkedModuleCall()
{
    uint32 searchAddress = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
	uint32 libraryStubAddr = m_cpu.m_pMemoryMap->GetWord(searchAddress + 0x0C);

	LIBRARYSTUB* libraryStub = reinterpret_cast<LIBRARYSTUB*>(m_ram + libraryStubAddr);

	const char* moduleName = reinterpret_cast<char*>(m_ram + libraryStub->moduleStrAddr);

	uint32 nidEntryAddr = libraryStub->stubNidTableAddr + (m_cpu.m_State.nGPR[CMIPS::V1].nV0 * 4);
	uint32 nid = *reinterpret_cast<uint32*>(m_ram + nidEntryAddr);

	ModuleMapType::const_iterator moduleIterator(m_modules.find(moduleName));
	if(moduleIterator == m_modules.end())
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOGNAME, "Module named '%s' wasn't found.\r\n", moduleName);
#endif
		return;
	}

	const ModulePtr& module = moduleIterator->second;
	module->Invoke(nid, m_cpu);
}

void CBios::HandleException()
{
    m_rescheduleNeeded = false;

    uint32 searchAddress = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
    uint32 callInstruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
    if(callInstruction == 0x0000000C)
    {
        switch(m_cpu.m_State.nGPR[CMIPS::V0].nV0)
        {
		case 0x666:
			HandleLinkedModuleCall();
			break;
		case 0x667:
			ExitCurrentThread(0);
			break;
		default:
			assert(0);
			break;
        }
    }
    else
    {
#ifdef _DEBUG
        CLog::GetInstance().Print(LOGNAME, "%0.8X: Unknown exception reason.\r\n", m_cpu.m_State.nCOP0[CCOP_SCU::EPC]);
#endif
		assert(0);
	}

    if(m_rescheduleNeeded)
    {
		assert((m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL) == 0);
		m_rescheduleNeeded = false;
		Reschedule();
    }

    m_cpu.m_State.nHasException = 0;	
}

MipsModuleList CBios::GetModuleList()
{
	return m_moduleTags;
}

void CBios::LoadModule(const char* path)
{
	{
		Framework::CStdStream stream(path, "rb");
		assert(m_module == NULL);
		m_module = new CElfFile(stream);
	}

	const ELFHEADER& moduleHeader(m_module->GetHeader());

	uint32 moduleAllocSize = 0;
	{
		for(unsigned int i = 0; i < moduleHeader.nProgHeaderCount; i++)
		{
			ELFPROGRAMHEADER* programHeader = m_module->GetProgram(i);
			if(programHeader->nType == RELOC_SECTION_ID) continue;
			//Allocate a bit more for alignment computations later on
			moduleAllocSize += programHeader->nFileSize + programHeader->nAlignment;
		}
	}

	uint32 baseAddress = Heap_AllocateMemory(moduleAllocSize);
	uint32 endAddress = 0;

	{
		uint32 currentAddress = baseAddress;
		for(unsigned int i = 0; i < moduleHeader.nProgHeaderCount; i++)
		{
			ELFPROGRAMHEADER* programHeader = m_module->GetProgram(i);
			if(programHeader->nType == RELOC_SECTION_ID) continue;

			uint32 alignFixUp = (currentAddress & (programHeader->nAlignment - 1));
			if(alignFixUp != 0)
			{
				currentAddress += programHeader->nAlignment - alignFixUp;
			}

			programHeader->nVAddress = currentAddress;

			memcpy(
				m_ram + currentAddress, 
				m_module->GetContent() + programHeader->nOffset, 
				programHeader->nFileSize);

			endAddress = std::max<uint32>(endAddress, currentAddress + programHeader->nFileSize);
			currentAddress += programHeader->nFileSize;
		}
	}

	*reinterpret_cast<uint32*>(m_ram + 0x0001323C) = 0x10400160;
	*reinterpret_cast<uint32*>(m_ram + 0x000137C0) = 0x100000CD;
	*reinterpret_cast<uint32*>(m_ram + 0x000137FC) = 0x1000FE0C;
	*reinterpret_cast<uint32*>(m_ram + 0x0001032C) = 0x0C0000D3;

	RelocateElf(*m_module);

    {
        MIPSMODULE module;
        module.name		= "main";
        module.begin	= baseAddress;
        module.end		= endAddress;
		module.param	= m_module;
        m_moduleTags.push_back(module);
    }

	ELFSECTIONHEADER* moduleInfoSectionHeader = m_module->FindSection(".rodata.sceModuleInfo");
	MODULEINFO* moduleInfo = reinterpret_cast<MODULEINFO*>(m_ram + baseAddress + moduleInfoSectionHeader->nStart);

	LIBRARYENTRY* libraryEntryBegin = reinterpret_cast<LIBRARYENTRY*>(m_ram + moduleInfo->libEntAddr);
	LIBRARYENTRY* libraryEntryEnd = reinterpret_cast<LIBRARYENTRY*>(m_ram + moduleInfo->libEntBtmAddr);

	LIBRARYSTUB* libraryStubBegin = reinterpret_cast<LIBRARYSTUB*>(m_ram + moduleInfo->libStubAddr);
	LIBRARYSTUB* libraryStubEnd = reinterpret_cast<LIBRARYSTUB*>(m_ram + moduleInfo->libStubBtmAddr);

	for(LIBRARYSTUB* libraryStub(libraryStubBegin); libraryStub != libraryStubEnd; libraryStub++)
	{
		const char* moduleName = reinterpret_cast<char*>(m_ram + libraryStub->moduleStrAddr);
		uint32 stubAddr = libraryStub->stubAddress;
		uint32 stubCount = libraryStub->stubSize;
		uint32 nidAddr = libraryStub->stubNidTableAddr;

		uint32 moduleTrampolineId = m_moduleTrampolines.Allocate();
		assert(moduleTrampolineId != -1);

		MODULETRAMPOLINE* trampoline(m_moduleTrampolines[moduleTrampolineId]);

		//ADDIU V0, R0, 0x666
		//SYSCALL
		//JR RA
		//NOP

		trampoline->code0		= ((0x09) << 26) | (CMIPS::R0 << 21) | (CMIPS::V0 << 16) | 0x666;
		trampoline->code1		= 0xC;
		trampoline->code2		= 0x03E00008;
		trampoline->code3		= 0;
		trampoline->libStubAddr = reinterpret_cast<uint8*>(libraryStub) - m_ram;

		uint32 trampolineAddr = (reinterpret_cast<uint8*>(trampoline) - m_ram) + 4;

		MODULE* moduleDef = FindModule(moduleName);
		for(unsigned int j = 0; j < stubCount; j++)
		{
			uint32* stubPtr = reinterpret_cast<uint32*>(m_ram + stubAddr + (j * 8));
			uint32 nid = *reinterpret_cast<uint32*>(m_ram + nidAddr + (j * 4));
			MODULEFUNCTION* moduleFunctionDef = FindModuleFunction(moduleDef, nid);
			std::string name;
			if(moduleFunctionDef != NULL)
			{
				name = moduleFunctionDef->name;
			}
			else
			{
				name = std::string(moduleName) + std::string("_") + lexical_cast_hex<std::string>(nid, 8);
			}
			m_cpu.m_Functions.InsertTag(stubAddr + (j * 8), name.c_str());

			//J $trampoline
			//ADDIU V1, R0, j

			*(stubPtr + 0) = ((0x02) << 26) | ((trampolineAddr >> 2) & 0x03FFFFFF);
			*(stubPtr + 1) = ((0x09) << 26) | (CMIPS::R0 << 21) | (CMIPS::V1 << 16) | j;
		}
	}

	m_cpu.m_State.nGPR[CMIPS::GP].nV0 = moduleInfo->gp;

	uint32 threadId = CreateThread("start_thread", baseAddress + moduleHeader.nEntryPoint, 0x20, DEFAULT_STACKSIZE, 0, 0);
	assert(threadId != -1);

    StartThread(threadId, 0, NULL);
    if(CurrentThreadId() == -1)
    {
        Reschedule();
    }

#if 0
	//Search string
//	const char* stringToSearch = "xa_MYROOM.adx";
	const char* stringToSearch = "fname:%s";
	size_t length = strlen(stringToSearch);
	for(unsigned int i = baseAddress; i < endAddress; i++)
	{
		if(!strncmp(stringToSearch, reinterpret_cast<const char*>(m_ram + i), length))
		{
			printf("Found string at %0x%0.8X.\r\n", i);
		}
	}
#endif
#if 0
	//0x003fe11c
	//0x003fe9f0
	//0x004428e4
	//0x00442954
	//0x00457988
	//0x1D96A0
	//0x3fdf4c
	//0x003fe9f0
	//1f5164
	//0x4bcd38
	//0x0040feac
	//0x0044dad0
	//0x004b9674
	for(unsigned int i = baseAddress; i < endAddress; i += 4)
	{
		uint32 opcode = *reinterpret_cast<uint32*>(m_ram + i);
		//if((opcode & 0xFFFF) == (0x8a50))
		//if(opcode == 0x004579e0)
		if(((opcode & 0xFFFF) == 0x4942))
		{
			printf("Found instruction at %0.8X.\r\n", i);
		}
	}
#endif
}

uint32 CBios::CreateThread(const char* name, uint32 threadProc, uint32 initPriority, uint32 stackSize, uint32 creationFlags, uint32 optParam)
{
	uint32 threadId = m_threads.Allocate();
	assert(threadId != -1);
	if(threadId == -1)
	{
		return -1;
	}

	assert(stackSize >= 512);
	uint32 stackBase = Heap_AllocateMemory(stackSize);

	THREAD* thread(m_threads[threadId]);
	strncpy(thread->name, name, 0x1F);
	thread->name[0x1F] = 0;

	thread->stackBase			= stackBase;
	thread->stackSize			= stackSize;
	thread->id					= threadId;
    thread->priority			= initPriority;
    thread->status				= THREAD_STATUS_CREATED;
    thread->nextActivateTime	= 0;
	thread->waitSemaphore		= -1;

	memset(&thread->context, 0, sizeof(THREADCONTEXT));
	memset(m_ram + thread->stackBase, 0, thread->stackSize);

    thread->context.epc = threadProc;
	thread->context.delayJump = 1;
	thread->context.gpr[CMIPS::RA] = m_threadFinishAddress;
    thread->context.gpr[CMIPS::SP] = thread->stackBase + thread->stackSize;
    thread->context.gpr[CMIPS::GP] = m_cpu.m_State.nGPR[CMIPS::GP].nV0;

	LinkThread(thread->id);
    return thread->id;
}

void CBios::StartThread(uint32 threadId, uint32 argsSize, uint8* argsPtr)
{
    THREAD& thread = GetThread(threadId);
    if(thread.status != THREAD_STATUS_CREATED)
    {
		throw std::runtime_error("Invalid thread state.");
    }
    thread.status = THREAD_STATUS_RUNNING;
    if(argsPtr != NULL)
    {
		//Copy params in stack
		assert(argsSize <= thread.stackSize);
		uint32 sp = thread.context.gpr[CMIPS::SP];
		sp -= argsSize;
		memcpy(m_ram + sp, argsPtr, argsSize);
		thread.context.gpr[CMIPS::SP] = sp;
        thread.context.gpr[CMIPS::A0] = sp;
    }
    m_rescheduleNeeded = true;
}

void CBios::ExitCurrentThread(uint32 exitCode)
{
	THREAD& thread = GetThread(CurrentThreadId());
	if(thread.status != THREAD_STATUS_RUNNING)
	{
		throw std::runtime_error("Invalid thread state.");
	}
	thread.status = THREAD_STATUS_ZOMBIE;
	m_rescheduleNeeded = true;
}

void CBios::LinkThread(uint32 threadId)
{
	THREAD* thread = m_threads[threadId];
	uint32* nextThreadId = &ThreadLinkHead();
	while(1)
	{
		if((*nextThreadId) == 0)
		{
			(*nextThreadId) = threadId;
			thread->nextThreadId = 0;
			break;
		}
		THREAD* currentThread = m_threads[(*nextThreadId)];
		if(currentThread->priority < thread->priority)
		{
			thread->nextThreadId = (*nextThreadId);
			(*nextThreadId) = threadId;
			break;
		}
		nextThreadId = &currentThread->nextThreadId;
	}
}

void CBios::UnlinkThread(uint32 threadId)
{
	THREAD* thread = m_threads[threadId];
	uint32* nextThreadId = &ThreadLinkHead();
	while(1)
	{
		if((*nextThreadId) == 0)
		{
			break;
		}
		THREAD* currentThread = m_threads[(*nextThreadId)];
		if((*nextThreadId) == threadId)
		{
			(*nextThreadId) = thread->nextThreadId;
			thread->nextThreadId = 0;
			break;
		}
		nextThreadId = &currentThread->nextThreadId;
	}
}

uint32& CBios::ThreadLinkHead() const
{
    return *reinterpret_cast<uint32*>(m_ram + BIOS_THREAD_LINK_HEAD_BASE);
}

uint32& CBios::CurrentThreadId() const
{
    return *reinterpret_cast<uint32*>(m_ram + BIOS_CURRENT_THREAD_ID_BASE);
}

CBios::THREAD& CBios::GetThread(uint32 threadId)
{
    return *m_threads[threadId];
}

void CBios::LoadThreadContext(uint32 threadId)
{
    THREAD& thread = GetThread(threadId);
    for(unsigned int i = 0; i < 32; i++)
    {
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
        m_cpu.m_State.nGPR[i].nD0 = static_cast<int32>(thread.context.gpr[i]);
    }
    m_cpu.m_State.nPC = thread.context.epc;
    m_cpu.m_State.nDelayedJumpAddr = thread.context.delayJump;
}

void CBios::SaveThreadContext(uint32 threadId)
{
    THREAD& thread = GetThread(threadId);
    for(unsigned int i = 0; i < 32; i++)
    {
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
        thread.context.gpr[i] = m_cpu.m_State.nGPR[i].nV0;
    }
    thread.context.epc = m_cpu.m_State.nPC;
    thread.context.delayJump = m_cpu.m_State.nDelayedJumpAddr;
}

uint32 CBios::GetNextReadyThread()
{
	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
        THREAD* nextThread = m_threads[nextThreadId];
		nextThreadId = nextThread->nextThreadId;
        if(GetCurrentTime() <= nextThread->nextActivateTime) continue;
        if(nextThread->status == THREAD_STATUS_RUNNING)
        {
            return nextThread->id;
        }
	}
	return -1;
}

void CBios::Reschedule()
{
    if(CurrentThreadId() != -1)
    {
        SaveThreadContext(CurrentThreadId());
		UnlinkThread(CurrentThreadId());
		LinkThread(CurrentThreadId());
    }

    uint32 nextThreadId = GetNextReadyThread();
    if(nextThreadId == -1)
    {
#ifdef _DEBUG
//		printf("Warning, no thread available for running.\r\n");
#endif
		m_cpu.m_State.nPC = m_idleFunctionAddress;
    }
	else
	{
		LoadThreadContext(nextThreadId);
	}
#ifdef _DEBUG
	if(nextThreadId != CurrentThreadId())
	{
		CLog::GetInstance().Print(LOGNAME, "Switched over to thread %i.\r\n", nextThreadId);
	}
#endif
	CurrentThreadId() = nextThreadId;
	m_cpu.m_nQuota = 1;
}

void CBios::Heap_Init()
{
	m_heapBegin	= CONTROL_BLOCK_END;
	m_heapEnd	= m_ramSize;
	m_heapSize	= m_heapEnd - m_heapBegin;

	m_heapBlocks.FreeAll();

    //Initialize block map
	m_heapHeadBlockId = m_heapBlocks.Allocate();
	HEAPBLOCK* block = m_heapBlocks[m_heapHeadBlockId];
	block->address		= m_heapSize;
	block->size			= 0;
	block->nextBlock	= 0;
}

uint32 CBios::Heap_AllocateMemory(uint32 size)
{
    uint32 begin = 0;
    const uint32 blockSize = MIN_HEAPBLOCK_SIZE;
    size = ((size + (blockSize - 1)) / blockSize) * blockSize;

	uint32* nextBlockId = &m_heapHeadBlockId;
	HEAPBLOCK* nextBlock = m_heapBlocks[*nextBlockId];
	while(nextBlock != NULL)
	{
		uint32 end = nextBlock->address;
		if((end - begin) >= size)
		{
			break;
		}
		begin = nextBlock->address + nextBlock->size;
		nextBlockId = &nextBlock->nextBlock;
		nextBlock = m_heapBlocks[*nextBlockId];
	}
	
	if(nextBlock != NULL)
	{
		uint32 newBlockId = m_heapBlocks.Allocate();
		assert(newBlockId != 0);
		if(newBlockId == 0)
		{
			return 0;
		}
		HEAPBLOCK* newBlock = m_heapBlocks[newBlockId];
		newBlock->address	= begin;
		newBlock->size		= size;
		newBlock->nextBlock	= *nextBlockId;
		*nextBlockId = newBlockId;
        return begin + m_heapBegin;
	}

    assert(0);
    return NULL;
}

uint32 CBios::Heap_FreeMemory(uint32 address)
{
    address -= m_heapBegin;
	//Search for block pointing at the address
	uint32* nextBlockId = &m_heapHeadBlockId;
	HEAPBLOCK* nextBlock = m_heapBlocks[*nextBlockId];
	while(nextBlock != NULL)
	{
		if(nextBlock->address == address)
		{
			break;
		}
		nextBlockId = &nextBlock->nextBlock;
		nextBlock = m_heapBlocks[*nextBlockId];
	}

	if(nextBlock != NULL)
	{
		m_heapBlocks.Free(*nextBlockId);
		*nextBlockId = nextBlock->nextBlock;
	}
	else
	{
		assert(0);
//		CLog::GetInstance().Print(LOG_NAME, "%s: Trying to unallocate an unexisting memory block (0x%0.8X).\r\n", __FUNCTION__, address);
	}
    return 0;
}

uint32 CBios::Heap_GetBlockId(uint32 address)
{
    address -= m_heapBegin;

	//Search for block pointing at the address
	uint32* nextBlockId = &m_heapHeadBlockId;
	HEAPBLOCK* nextBlock = m_heapBlocks[*nextBlockId];
	while(nextBlock != NULL)
	{
		if(nextBlock->address == address)
		{
			break;
		}
		nextBlockId = &nextBlock->nextBlock;
		nextBlock = m_heapBlocks[*nextBlockId];
	}

	if(nextBlock != NULL)
	{
		return *nextBlockId;
	}
	else
	{
		return -1;
	}
}

uint32 CBios::Heap_GetBlockAddress(uint32 blockId)
{
	HEAPBLOCK* block = m_heapBlocks[blockId];
	assert(block != NULL);
	if(block == NULL)
	{
		return 0;
	}
	return block->address + m_heapBegin;
}

CBios::MODULE* CBios::FindModule(const char* name)
{
	MODULE* currentModule = g_modules;
	while(currentModule->name != NULL)
	{
		if(!strcmp(currentModule->name, name))
		{
			return currentModule;
		}
		currentModule++;
	}
	return NULL;
}

CBios::MODULEFUNCTION* CBios::FindModuleFunction(MODULE* module, uint32 id)
{
	if(module == NULL) return NULL;
	MODULEFUNCTION* currentFunction = module->functions;
	while(currentFunction->name != NULL)
	{
		if(currentFunction->id == id)
		{
			return currentFunction;
		}
		currentFunction++;
	}
	return NULL;
}

void CBios::RelocateElf(CELF& elf)
{
    const ELFHEADER& header = elf.GetHeader();
    for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
    {
        ELFSECTIONHEADER* sectionHeader = elf.GetSection(i);
        if(sectionHeader != NULL && 
			(/*sectionHeader->nType == CELF::SHT_REL || */sectionHeader->nType == RELOC_SECTION_ID)
			)
        {
            unsigned int linkedSection = sectionHeader->nInfo;
            unsigned int recordCount = sectionHeader->nSize / 8;
            const uint32* relocationRecord = reinterpret_cast<const uint32*>(elf.GetSectionData(i));
            if(relocationRecord == NULL) continue;
            for(unsigned int record = 0; record < recordCount; record++)
            {
                uint32 relocationType = relocationRecord[1] & 0xFF;
				uint32 ofsBase = (relocationRecord[1] >> 8) & 0xFF;
				uint32 addrBase = (relocationRecord[1] >> 16) & 0xFF;

				ELFPROGRAMHEADER* progOfsBase = elf.GetProgram(ofsBase);
				ELFPROGRAMHEADER* progAddrBase = elf.GetProgram(addrBase);
				assert(progOfsBase != NULL);
				assert(progAddrBase != NULL);
				assert(progOfsBase->nType != RELOC_SECTION_ID);
				assert(progAddrBase->nType != RELOC_SECTION_ID);

				uint32 baseAddress = progAddrBase->nVAddress;
                uint32 relocationAddress = relocationRecord[0] + progOfsBase->nVAddress;

				assert(relocationAddress < m_ramSize);
				if(relocationAddress < m_ramSize)
                {
					uint32& instruction = *reinterpret_cast<uint32*>(m_ram + relocationAddress);
                    switch(relocationType)
                    {
                    case CELF::R_MIPS_32:
                        {
                            instruction += baseAddress;
                        }
                        break;
                    case CELF::R_MIPS_26:
                        {
                            uint32 offset = (instruction & 0x03FFFFFF) + (baseAddress >> 2);
                            instruction &= ~0x03FFFFFF;
                            instruction |= offset;
                        }
                        break;
                    case CELF::R_MIPS_HI16:
                        {
							//Find the next LO16 reloc
							uint32 nextRelocAddress = FindNextRelocationTarget(elf, relocationRecord + 2, relocationRecord + (recordCount - record) * 2);
							assert(nextRelocAddress != -1);
							if(nextRelocAddress != -1)
							{
								uint32 nextInstruction = *reinterpret_cast<uint32*>(m_ram + nextRelocAddress);
								uint32 offset = static_cast<int16>(nextInstruction) + (instruction << 16);
                                offset += baseAddress;
								instruction &= ~0xFFFF;
								if(offset & 0x8000) offset += 0x10000;
								instruction |= offset >> 16;
							}
                        }
                        break;
                    case CELF::R_MIPS_LO16:
                        {
                            uint32 offset = static_cast<int16>(instruction);
                            offset += baseAddress;
                            instruction &= ~0xFFFF;
                            instruction |= offset & 0xFFFF;
                        }
                        break;
                    default:
						throw std::runtime_error("Unknown relocation type.");
                        break;
                    }
                }
                relocationRecord += 2;
            }
        }
    }
}

uint32 CBios::FindNextRelocationTarget(CELF& elf, const uint32* begin, const uint32* end)
{
	for(const uint32* relocationRecord(begin); relocationRecord != end; relocationRecord += 2)
	{
		uint32 relocationType = relocationRecord[1] & 0xFF;
		uint32 ofsBase = (relocationRecord[1] >> 8) & 0xFF;
		uint32 addrBase = (relocationRecord[1] >> 16) & 0xFF;

		ELFPROGRAMHEADER* progOfsBase = elf.GetProgram(ofsBase);

		assert(progOfsBase != NULL);
		assert(progOfsBase->nType != RELOC_SECTION_ID);

		if(relocationType == CELF::R_MIPS_LO16)
		{
			return relocationRecord[0] + progOfsBase->nVAddress;
		}
	}
	return -1;
}
