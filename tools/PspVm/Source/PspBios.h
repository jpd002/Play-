#ifndef _PSPBIOS_H_
#define _PSPBIOS_H_

#include <map>
#include "Types.h"
#include "ELF.h"
#include "MIPS.h"
#include "MIPSModule.h"
#include "OsStructManager.h"
#include "PspModule.h"

namespace Psp
{
	class CBios
	{
	public:
						CBios(CMIPS&, uint8*, uint32);
		virtual			~CBios();

		void			Reset();
		void			LoadModule(const char*);
		MipsModuleList	GetModuleList();

		void			HandleException();

		uint32			CreateThread(const char*, uint32, uint32, uint32, uint32, uint32);
	    void			StartThread(uint32, uint32, uint8*);

	private:
		enum CONTROL_BLOCK
		{
			CONTROL_BLOCK_START = 0x10,
			CONTROL_BLOCK_END   = 0x10000,
		};

		struct HEAPBLOCK
		{
			uint32	isValid;
			uint32	nextBlock;
			uint32	address;
			uint32	size;
		};

		struct THREADCONTEXT
		{
			uint32      gpr[0x20];
			uint32      epc;
			uint32      delayJump;
		};

		struct THREAD
		{
			uint32			isValid;
			uint32          id;
			char			name[0x20];
			uint32          priority;
			THREADCONTEXT   context;
			uint32          status;
			uint32          waitSemaphore;
			uint32          wakeupCount;
			uint32			stackBase;
			uint32			stackSize;
			uint32			nextThreadId;
			uint64          nextActivateTime;
		};

		enum
		{
			MAX_HEAPBLOCKS = 256,
			MIN_HEAPBLOCK_SIZE = 0x20,
		};

		enum
		{
			MAX_MODULETRAMPOLINES = 128,
		};

		enum
		{
			MAX_THREADS = 32,
		};

		enum DEFAULT_STACKSIZE
		{
			DEFAULT_STACKSIZE = 0x10000,
		};

		enum THREAD_STATUS
		{
			THREAD_STATUS_CREATED,
			THREAD_STATUS_RUNNING,
		};

		struct LIBRARYENTRY
		{
			uint32	libraryAddress;
			uint16	version;
			uint16	attributes;
			uint8	exportEntryCount;
			uint8	variableCount;
			uint16	functionCount;
			uint32	entryTableAddress;
		};

		struct LIBRARYSTUB
		{
			uint32	moduleStrAddr;
			uint16	importFlags;
			uint16	libraryVersion;
			uint16	stubCount;
			uint16	stubSize;
			uint32	stubNidTableAddr;
			uint32	stubAddress;
		};

		struct MODULEINFO
		{
			uint16	attributes;
			uint16	version;
			uint8	name[28];
			uint32	gp;
			uint32	libEntAddr;
			uint32	libEntBtmAddr;
			uint32	libStubAddr;
			uint32	libStubBtmAddr;
		};

		struct MODULEFUNCTION
		{
			uint32			id;
			const char*		name;
		};

		struct MODULE
		{
			const char*			name;
			MODULEFUNCTION*		functions;
		};

		struct MODULETRAMPOLINE
		{
			uint32	isValid;
			uint32	code0;
			uint32	code1;
			uint32	code2;
			uint32	code3;
			uint32	libStubAddr;
		};

		typedef std::tr1::shared_ptr<CModule> ModulePtr;
		typedef std::map<std::string, ModulePtr> ModuleMapType;
		typedef COsStructManager<HEAPBLOCK> HeapBlockListType;
		typedef COsStructManager<MODULETRAMPOLINE> ModuleTrampolineListType;
		typedef COsStructManager<THREAD> ThreadListType;

		void						InsertModule(const ModulePtr&);
		void						HandleLinkedModuleCall();

		MODULE*						FindModule(const char*);
		MODULEFUNCTION*				FindModuleFunction(MODULE*, uint32);
		void						RelocateElf(CELF&);
		uint32						FindNextRelocationTarget(CELF&, const uint32*, const uint32*);

		void						LinkThread(uint32);
		void						UnlinkThread(uint32);
		THREAD&						GetThread(uint32);
		void						LoadThreadContext(uint32);
		void						SaveThreadContext(uint32);
		void						Reschedule();
	    uint32						GetNextReadyThread();

		uint32&						ThreadLinkHead() const;
		uint32&						CurrentThreadId() const;

		void						Heap_Init();
        uint32						Heap_AllocateMemory(uint32);
        uint32						Heap_FreeMemory(uint32);

		CELF*						m_module;
		uint8*						m_ram;
		uint32						m_ramSize;
		CMIPS&						m_cpu;

		ModuleMapType				m_modules;
		MipsModuleList				m_moduleTags;

		HeapBlockListType			m_heapBlocks;
        uint32						m_heapBegin;
        uint32						m_heapEnd;
        uint32						m_heapSize;
		uint32						m_heapHeadBlockId;

		uint32						m_threadFinishAddress;
		uint32						m_idleFunctionAddress;

		ModuleTrampolineListType	m_moduleTrampolines;
		ThreadListType				m_threads;

		static MODULEFUNCTION		g_IoFileMgrForUserFunctions[];
		static MODULEFUNCTION		g_SysMemUserForUserFunctions[];
		static MODULEFUNCTION		g_ThreadManForUserFunctions[];
		static MODULEFUNCTION		g_LoadExecForUserFunctions[];
		static MODULEFUNCTION		g_UtilsForUserFunctions[];
		static MODULEFUNCTION		g_SasCoreFunctions[];
		static MODULEFUNCTION		g_WaveFunctions[];
		static MODULEFUNCTION		g_UmdUserFunctions[];
		static MODULEFUNCTION		g_UtilityFunctions[];
		static MODULEFUNCTION		g_KernelLibraryFunctions[];
		static MODULEFUNCTION		g_ModuleMgrForUserFunctions[];
		static MODULEFUNCTION		g_StdioForUserFunctions[];
		static MODULE				g_modules[];

		bool						m_rescheduleNeeded;
	};
}

#endif
