#ifndef _PS2VM_H_
#define _PS2VM_H_

#include <boost/thread.hpp>
#include "AppDef.h"
#include "Types.h"
#include "DMAC.h"
#include "GIF.h"
#include "SIF.h"
#include "VIF.h"
#include "IPU.h"
#include "INTC.h"
#include "Timer.h"
#include "MIPS.h"
#include "MailBox.h"
#include "GSHandler.h"
#include "PadHandler.h"
#include "iso9660/ISO9660.h"
#include "VirtualMachine.h"
#include "MipsExecutor.h"
#include "MA_VU.h"
#include "MA_EE.h"
#include "COP_SCU.h"
#include "COP_FPU.h"
#include "COP_VU.h"
#include "iop/Iop_SubSystem.h"
#include "PS2OS.h"

class CIopBios;

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

#ifdef	PROFILE
#define	PROFILE_EEZONE "EE"
#endif

#define PREF_PS2_HOST_DIRECTORY				("ps2.host.directory")
#define PREF_PS2_MC0_DIRECTORY				("ps2.mc0.directory")
#define PREF_PS2_MC1_DIRECTORY				("ps2.mc1.directory")
#define PREF_PS2_FRAMESKIP					("ps2.frameskip")

class CPS2VM : public CVirtualMachine
{
public:
								CPS2VM();
	virtual						~CPS2VM();

	void						Initialize();
	void						Destroy();

	void						StepEe();
	void						StepIop();
	void						StepVu1();

	void						Resume();
	void						Pause();
	void						Reset();

	STATUS						GetStatus() const;

	void						DumpEEIntcHandlers();
	void						DumpEEDmacHandlers();

	void						CreateGSHandler(const CGSHandler::FactoryFunction&);
	CGSHandler*					GetGSHandler();
	void						DestroyGSHandler();

	void						CreatePadHandler(const CPadHandler::FactoryFunction&);
	void						DestroyPadHandler();

	unsigned int				SaveState(const char*);
	unsigned int				LoadState(const char*);

#ifdef DEBUGGER_INCLUDED
	std::string					MakeDebugTagsPackagePath(const char*);
	void						LoadDebugTags(const char*);
	void						SaveDebugTags(const char*);

	bool						IsSaveVpuStateEnabled() const;
	void						SetSaveVpuStateEnabled(bool);
#endif

	void						SetFrameSkip(unsigned int);

	uint32						IOPortReadHandler(uint32);
	uint32						IOPortWriteHandler(uint32, uint32);

	uint32						Vu0IoPortReadHandler(uint32);
	uint32						Vu0IoPortWriteHandler(uint32, uint32);

	uint32						Vu1IoPortReadHandler(uint32);
	uint32						Vu1IoPortWriteHandler(uint32, uint32);

	CGSHandler*					m_gs;
	CPadHandler*				m_pad;

	Iop::CSubSystem				m_iop;

	uint8*						m_ram;
	uint8*						m_bios;
	uint8*						m_spr;
	uint8*						m_fakeIopRam;

	uint8*						m_pVUMem0;
	uint8*						m_pMicroMem0;

	uint8*						m_pVUMem1;
	uint8*						m_pMicroMem1;

	CDMAC						m_dmac;
	CGIF						m_gif;
	CSIF						m_sif;
	CVIF						m_vif;
	CINTC						m_intc;
	CIPU						m_ipu;
	CTimer						m_timer;
	CPS2OS*						m_os;
	CIopBios*					m_iopOs;
	Iop::CSubSystem::BiosPtr	m_iopOsPtr;

	CMIPS						m_EE;
	CMIPS						m_VU0;
	CMIPS						m_VU1;
	CMipsExecutor				m_executor;

	int							m_vblankTicks;
	bool						m_inVblank;
	int							m_spuUpdateTicks;

	CISO9660*					m_pCDROM0;

private:
	void						CreateVM();
	void						ResetVM();
	void						DestroyVM();
	void						SaveVMState(const char*, unsigned int&);
	void						LoadVMState(const char*, unsigned int&);

	void						EEMemWriteHandler(uint32);
	void						FlushInstructionCache();
	void						ReloadExecutable(const char*, const CPS2OS::ArgumentList&);

	void						ResumeImpl();
	void						PauseImpl();
	void						DestroyImpl();
	void						CreateGsImpl(const CGSHandler::FactoryFunction&);
	void						DestroyGsImpl();

	void						CreatePadHandlerImpl(const CPadHandler::FactoryFunction&);
	void						DestroyPadHandlerImpl();

	void						UpdateSpu();
	void						OnGsNewFrame();

	void						CDROM0_Initialize();
	void						CDROM0_Mount(const char*);
	void						CDROM0_Reset();
	void						CDROM0_Destroy();
	void						SetIopCdImage(CISO9660*);

	void						LoadBIOS();
	void						RegisterModulesInPadHandler();
	void						FillFakeIopRam();

	void						EmuThread();

	boost::thread*				m_thread;
	CMailBox					m_mailBox;
	STATUS						m_nStatus;
	bool						m_nEnd;
	CMA_VU						m_MAVU0;
	CMA_VU						m_MAVU1;
	CMA_EE						m_EEArch;
	CCOP_SCU					m_COP_SCU;
	CCOP_FPU					m_COP_FPU;
	CCOP_VU						m_COP_VU;
	unsigned int				m_frameNumber;
	unsigned int				m_frameSkip;
	bool						m_singleStepEe;
	bool						m_singleStepIop;
	bool						m_singleStepVu0;
	bool						m_singleStepVu1;

#ifdef DEBUGGER_INCLUDED
	bool						m_saveVpuStateEnabled;
#endif
};

#endif
