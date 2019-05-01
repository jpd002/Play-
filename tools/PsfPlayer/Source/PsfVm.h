#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "Types.h"
#include "MIPS.h"
#include "SoundHandler.h"
#include "PsfVmSubSystem.h"
#include "VirtualMachine.h"
#include "Debuggable.h"
#include "MailBox.h"
#include <thread>

class CPsfVm : public CVirtualMachine, public boost::signals2::trackable
{
public:
	typedef std::function<CSoundHandler*()> SpuHandlerFactory;
	typedef boost::signals2::signal<void()> OnNewFrameEvent;

	CPsfVm();
	virtual ~CPsfVm();

	void Reset();
	void Step();
	void SetSpuHandler(const SpuHandlerFactory&);

	void SetReverbEnabled(bool);
	void SetVolumeAdjust(float);

	CMIPS& GetCpu();
	uint8* GetRam();
	Iop::CSpuBase& GetSpuCore(unsigned int);

	void SetSubSystem(const PsfVmSubSystemPtr&);

	CDebuggable GetDebugInfo();

	STATUS GetStatus() const override;
	void Pause() override;
	void Resume() override;

#ifdef DEBUGGER_INCLUDED
	std::string MakeTagPackagePath(const char*);
	void LoadDebugTags(const char*);
	void SaveDebugTags(const char*);
#endif

	OnNewFrameEvent OnNewFrame;

private:
	void ThreadProc();

	void SetReverbEnabledImpl(bool);
	void SetVolumeAdjustImpl(float);
	void PauseImpl();
	void SetSpuHandlerImpl(const SpuHandlerFactory&);

	STATUS m_status;
	PsfVmSubSystemPtr m_subSystem;
	CSoundHandler* m_soundHandler;
	std::thread m_thread;
	bool m_singleStep;
	bool m_isThreadOver;
	CMailBox m_mailBox;
};

#endif
