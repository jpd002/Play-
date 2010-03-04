#ifndef _PSPVM_H_
#define _PSPVM_H_

#include "PspBios.h"
#include "Types.h"
#include "MA_ALLEGREX.h"
#include "COP_SCU.h"
#include "COP_FPU.h"
#include "MIPS.h"
#include "MIPSExecutor.h"
#include "Debuggable.h"
#include "VirtualMachine.h"
#include "MailBox.h"
#include <boost/thread.hpp>

class CPspVm : public CVirtualMachine
{
public:
						CPspVm();
	virtual				~CPspVm();

	STATUS				GetStatus() const;
	void				Pause();
	void				Resume();
	void				Reset();

	void				LoadModule(const char*);

	CDebuggable			GetDebugInfo();

	void				LoadDebugTags(const char*);
	void				SaveDebugTags(const char*);

	void				Step();
	CMIPS&				GetCpu();

private:
	std::string			MakeTagPackagePath(const char*);
	void				ThreadProc();

	CMIPS				m_cpu;
	CMipsExecutor		m_executor;
	CMA_ALLEGREX		m_cpuArch;
	CCOP_SCU			m_copScu;
	CCOP_FPU			m_copFpu;
	uint8*				m_ram;
	Psp::CBios			m_bios;

	CMailBox			m_mailBox;
	STATUS				m_status;
	bool				m_singleStep;
	bool				m_isThreadOver;
	boost::thread*		m_thread;
};

#endif
