#pragma once

#include "../../VirtualMachine.h"
#include "../../MIPS.h"
#include "../../MailBox.h"
#include "../../ee/MA_VU.h"
#include "../../ee/VUExecutor.h"

class CVu1Vm : public CVirtualMachine
{
public:
						CVu1Vm();
	virtual				~CVu1Vm();

	void				Pause() override;
	void				Resume() override;
	STATUS				GetStatus() const override;

	void				Reset();

	void				StepVu1();
	CMIPS*				GetVu1Context();

	uint8*				GetMicroMem1();
	uint8*				GetVuMem1();

	void				SetVpu1Top(uint32);
	void				SetVpu1Itop(uint32);

private:
	uint32				Vu1IoPortReadHandler(uint32);
	uint32				Vu1IoPortWriteHandler(uint32, uint32);

	void				ThreadProc();

//	CMailBox			m_mailBox;
//	std::thread			m_thread;
//	bool				m_threadDone;

	CMIPS				m_vu1;
	CVuExecutor			m_vu1Executor;
	uint8*				m_vuMem1;
	uint8*				m_microMem1;
	CMA_VU				m_maVu1;
	uint32				m_vpu1_TOP;
	uint32				m_vpu1_ITOP;

	STATUS				m_status;
};
