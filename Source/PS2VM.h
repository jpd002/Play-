#ifndef _PS2VM_H_
#define _PS2VM_H_

#include <boost/thread.hpp>
#include <boost/signal.hpp>
#include "Types.h"
#include "PS2OS.h"
#include "DMAC.h"
#include "GIF.h"
#include "MIPS.h"
#include "ThreadMsg.h"
#include "GSHandler.h"
#include "PadHandler.h"
#include "LogControl.h"
#include "iso9660/ISO9660.h"
#include "VirtualMachine.h"
#include "MipsExecutor.h"

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

#define VERSION_MAJOR		(0)
#define VERSION_MINOR		(20)

#ifdef	PROFILE
#define	PROFILE_EEZONE "EE"
#endif

typedef CGSHandler*			(*GSHANDLERFACTORY)(void*); 
typedef CPadHandler*		(*PADHANDLERFACTORY)(void*);

class CPS2VM : public CVirtualMachine
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

                                CPS2VM();
    virtual                     ~CPS2VM();

	void						Initialize();
	void						Destroy();

    void                        Step();
	void						Resume();
	void						Pause();
	void						Reset();

    STATUS                      GetStatus() const;

	void						DumpEEThreadSchedule();
	void						DumpEEIntcHandlers();
	void						DumpEEDmacHandlers();

	void						CreateGSHandler(GSHANDLERFACTORY, void*);
	CGSHandler*				    GetGSHandler();
	void						DestroyGSHandler();

	void						CreatePadHandler(PADHANDLERFACTORY, void*);
	void						DestroyPadHandler();

	unsigned int				SaveState(const char*);
	unsigned int				LoadState(const char*);

	uint32                      IOPortReadHandler(uint32);
	uint32                      IOPortWriteHandler(uint32, uint32);

	void                        EEMemWriteHandler(uint32);

	CGSHandler*				    m_pGS;
	CPadHandler*				m_pPad;

	uint8*					    m_pRAM;
	uint8*					    m_pBIOS;
	uint8*					    m_pSPR;

    CDMAC                       m_dmac;
    CGIF                        m_gif;
	CPS2OS*                     m_os;

	uint8*					    m_pVUMem0;
	uint8*					    m_pMicroMem0;

	uint8*					    m_pVUMem1;
	uint8*                      m_pMicroMem1;

    CMIPS                       m_EE;
    CMIPS                       m_VU1;
    CMipsExecutor               m_executor;
    unsigned int				m_nVBlankTicks;
    bool						m_nInVBlank;
    bool                        m_singleStep;

	boost::signal<void ()>      m_OnNewFrame;

	CLogControl				    m_Logging;

	CISO9660*				    m_pCDROM0;

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

	void						CreateVM();
	void						ResetVM();
	void						DestroyVM();
	unsigned int				SaveVMState(const char*);
	unsigned int				LoadVMState(const char*);

	unsigned int                EETickFunction(unsigned int);
	unsigned int                VU1TickFunction(unsigned int);
    static unsigned int         EETickFunctionStub(unsigned int, CMIPS*);
    static unsigned int         VU1TickFunctionStub(unsigned int, CMIPS*);
    static void                 EESysCallHandlerStub(CMIPS*);

	void						CDROM0_Initialize();
	void						CDROM0_Mount(const char*);
	void						CDROM0_Reset();
	void						CDROM0_Destroy();

	void						LoadBIOS();
	void						RegisterModulesInPadHandler();

	unsigned int				SendMessage(PS2VM_MSG, void* = NULL);
	void						EmuThread();
	boost::thread*			    m_pThread;
	CThreadMsg				    m_MsgBox;
    STATUS                      m_nStatus;
};

#endif
