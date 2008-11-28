#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "Types.h"
#include "MIPS.h"
#include "SH_OpenAL.h"
#include "iop/Iop_SpuBase.h"
#include "iop/Iop_Spu.h"
#include "iop/Iop_Spu2.h"
#include "iop/Iop_Dmac.h"
#include "iop/Iop_Intc.h"
#include "iop/Iop_RootCounters.h"
#include "iop/Iop_BiosBase.h"
#include "VirtualMachine.h"
#include "Debuggable.h"
#include "MailBox.h"
#include "MipsExecutor.h"
#include <boost/thread.hpp>
#include <boost/signal.hpp>

class CPsfVm : public CVirtualMachine
{
public:
	enum RAMSIZE
	{
		RAMSIZE = 0x00200000
	};

	enum SCRATCHSIZE
	{
		SCRATCHSIZE = 0x00000400,
	};

	enum SPURAMSIZE
	{
		SPURAMSIZE = 0x00200000,
	};

	enum
	{
		CLOCK_FREQ		= (44100 * 256 * 3)		//~33.8MHz
	};

						CPsfVm();
	virtual				~CPsfVm();

	void				Reset();
	void				Step();

	CMIPS&				GetCpu();
	Iop::CSpuBase&		GetSpuCore(unsigned int);
    uint8*              GetRam();

	void                SetBios(Iop::CBiosBase*);

    CDebuggable         GetDebugInfo();

	virtual STATUS		GetStatus() const;
    virtual void		Pause();
    virtual void		Resume();

#ifdef DEBUGGER_INCLUDED
	std::string			MakeTagPackagePath(const char*);
	void				LoadDebugTags(const char*);
	void				SaveDebugTags(const char*);
#endif

	boost::signal<void ()> OnNewFrame;

private:
	enum
	{
		HW_REG_BEGIN	= 0x1F801000,
		HW_REG_END		= 0x1F9FFFFF
	};

	unsigned int		ExecuteCpu(bool);
	void				ThreadProc();

	uint32				ReadIoRegister(uint32);
	uint32				WriteIoRegister(uint32, uint32);

	void				PauseImpl();

	STATUS				m_status;
	uint8*				m_ram;
	uint8*				m_scratchPad;
	uint8*				m_spuRam;
	Iop::CIntc			m_intc;
	Iop::CRootCounters	m_counters;
	Iop::CDmac			m_dmac;
	Iop::CSpuBase		m_spuCore0;
	Iop::CSpuBase		m_spuCore1;
	Iop::CSpu			m_spu;
	Iop::CSpu2			m_spu2;
	CMIPS				m_cpu;
	CMipsExecutor		m_executor;
	Iop::CBiosBase*     m_bios;
	CSH_OpenAL			m_spuHandler;
	boost::thread		m_thread;
	bool				m_singleStep;
    int                 m_spuUpdateCounter;
    int                 m_frameCounter;
	CMailBox			m_mailBox;
};

#endif
