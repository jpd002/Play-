#ifndef _PS2VM_H_
#define _PS2VM_H_

#include <boost/thread.hpp>
#include "Types.h"
#include "DMAC.h"
#include "GIF.h"
#include "SIF.h"
#include "VIF.h"
#include "IPU.h"
#include "INTC.h"
#include "MIPS.h"
#include "MailBox.h"
#include "GSHandler.h"
#include "PadHandler.h"
#include "iso9660/ISO9660.h"
#include "VirtualMachine.h"
#include "MipsExecutor.h"

class CIopBios;
class CPS2OS;

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

class CPS2VM : public CVirtualMachine
{
public:
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

    void						CreateGSHandler(const CGSHandler::FactoryFunction&);
	CGSHandler*				    GetGSHandler();
	void						DestroyGSHandler();

    void						CreatePadHandler(const CPadHandler::FactoryFunction&);
	void						DestroyPadHandler();

	unsigned int				SaveState(const char*);
	unsigned int				LoadState(const char*);

	uint32                      IOPortReadHandler(uint32);
	uint32                      IOPortWriteHandler(uint32, uint32);

    uint32                      Vu1IoPortReadHandler(uint32);
    uint32                      Vu1IoPortWriteHandler(uint32, uint32);

	void                        EEMemWriteHandler(uint32);

	CGSHandler*				    m_pGS;
	CPadHandler*				m_pPad;

	uint8*					    m_pRAM;
	uint8*					    m_pBIOS;
	uint8*					    m_pSPR;
    uint8*                      m_iopRam;

	uint8*					    m_pVUMem0;
	uint8*					    m_pMicroMem0;

	uint8*					    m_pVUMem1;
	uint8*                      m_pMicroMem1;

    CDMAC                       m_dmac;
    CGIF                        m_gif;
    CSIF                        m_sif;
    CVIF                        m_vif;
    CINTC                       m_intc;
    CIPU                        m_ipu;
	CPS2OS*                     m_os;
    CIopBios*                   m_iopOs;

    CMIPS                       m_EE;
    CMIPS                       m_VU1;
    CMIPS                       m_iop;
    CMipsExecutor               m_executor;
    unsigned int				m_nVBlankTicks;
    bool						m_nInVBlank;
    bool                        m_singleStep;

	CISO9660*				    m_pCDROM0;

private:
    void						CreateVM();
    void						ResetVM();
    void						DestroyVM();
    void                        SaveVMState(const char*, unsigned int&);
    void                        LoadVMState(const char*, unsigned int&);

    void                        ResumeImpl();
    void                        PauseImpl();
    void                        DestroyImpl();
    void                        CreateGsImpl(const CGSHandler::FactoryFunction&);
    void                        DestroyGsImpl();

    void                        CreatePadHandlerImpl(const CPadHandler::FactoryFunction&);
    void                        DestroyPadHandlerImpl();

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

//	unsigned int				SendMessage(PS2VM_MSG, void* = NULL);
	void						EmuThread();
	boost::thread*			    m_pThread;
//	CThreadMsg				    m_MsgBox;
    CMailBox                    m_mailBox;
    STATUS                      m_nStatus;
    bool                        m_nEnd;
};

#endif
