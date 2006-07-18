#ifndef _PS2VM_H_
#define _PS2VM_H_

#include <boost/thread.hpp>
#include "Types.h"
#include "PS2OS.h"
#include "MIPS.h"
#include "ThreadMsg.h"
#include "GSHandler.h"
#include "PadHandler.h"
#include "LogControl.h"
#include "Event.h"
#include "iso9660/ISO9660.h"

enum PS2VM_MSG
{
	PS2VM_MSG_PAUSE,
	PS2VM_MSG_RESUME,
	PS2VM_MSG_DESTROY,
	PS2VM_MSG_CREATEGS,
	PS2VM_MSG_DESTROYGS,
	PS2VM_MSG_CREATEPAD,
	PS2VM_MSG_DESTROYPAD,
	PS2VM_MSG_SAVESTATE,
	PS2VM_MSG_LOADSTATE,
	PS2VM_MSG_RESET,
};

enum PS2VM_STATUS
{
	PS2VM_STATUS_PAUSED,
	PS2VM_STATUS_RUNNING,
};

#define VERSION_MAJOR		(0)
#define VERSION_MINOR		(20)

#ifdef	PROFILE
#define	PROFILE_EEZONE "EE"
#endif

typedef CGSHandler*			(*GSHANDLERFACTORY)(void*); 
typedef CPadHandler*		(*PADHANDLERFACTORY)(void*);

class CPS2VM
{
public:
	enum PS2VM_RAMSIZE
	{
		RAMSIZE = 0x02000000,
	};

	enum PS2VM_BIOSSIZE
	{
		BIOSSIZE = 0x00400000,
	};

	enum PS2VM_SPRSIZE
	{
		SPRSIZE = 0x00004000,
	};

	enum PS2VM_VUMEM1SIZE
	{
		VUMEM1SIZE = 0x00004000,
	};

	enum PS2VM_MICROMEM1SIZE
	{
		MICROMEM1SIZE = 0x00004000,
	};

	static void						Initialize();
	static void						Destroy();

	static void						Resume();
	static void						Pause();
	static void						Reset();

	static void						DumpEEThreadSchedule();
	static void						DumpEEIntcHandlers();
	static void						DumpEEDmacHandlers();

	static void						CreateGSHandler(GSHANDLERFACTORY, void*);
	static CGSHandler*				GetGSHandler();
	static void						DestroyGSHandler();

	static void						CreatePadHandler(PADHANDLERFACTORY, void*);
	static void						DestroyPadHandler();

	static unsigned int				SaveState(const char*);
	static unsigned int				LoadState(const char*);

	static uint32					IOPortReadHandler(uint32);
	static void						IOPortWriteHandler(uint32, uint32);

	static CGSHandler*				m_pGS;
	static CPadHandler*				m_pPad;

	static uint8*					m_pRAM;
	static uint8*					m_pBIOS;
	static uint8*					m_pSPR;

	static uint8*					m_pVUMem0;
	static uint8*					m_pMicroMem0;

	static uint8*					m_pVUMem1;
	static uint8*					m_pMicroMem1;

	static CMIPS					m_EE;
	static CMIPS					m_VU1;
	static PS2VM_STATUS				m_nStatus;
	static unsigned int				m_nVBlankTicks;
	static bool						m_nInVBlank;

	static Framework::CEvent<int>	m_OnMachineStateChange;
	static Framework::CEvent<int>	m_OnRunningStateChange;
	static Framework::CEvent<int>	m_OnNewFrame;

	static CLogControl				m_Logging;

	static CISO9660*				m_pCDROM0;

private:
	struct CREATEGSHANDLERPARAM
	{
		GSHANDLERFACTORY			pFactory;
		void*						pParam;
	};

	struct CREATEPADHANDLERPARAM
	{
		PADHANDLERFACTORY			pFactory;
		void*						pParam;
	};

	static void						CreateVM();
	static void						ResetVM();
	static void						DestroyVM();
	static unsigned int				SaveVMState(const char*);
	static unsigned int				LoadVMState(const char*);

	static unsigned int				EETickFunction(unsigned int);
	static unsigned int				VU1TickFunction(unsigned int);

	static void						CDROM0_Initialize();
	static void						CDROM0_Mount(const char*);
	static void						CDROM0_Reset();
	static void						CDROM0_Destroy();

	static void						LoadBIOS();
	static void						RegisterModulesInPadHandler();

	static unsigned int				SendMessage(PS2VM_MSG, void* = NULL);
	static void						EmuThread();
	static boost::thread*			m_pThread;
	static CThreadMsg				m_MsgBox;

	static CPS2OS*					m_pOS;
};

#endif
