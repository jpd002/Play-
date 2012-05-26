#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "Types.h"
#include "MIPS.h"
#include "SoundHandler.h"
#include "PsfVmSubSystem.h"
#include "VirtualMachine.h"
#include "Debuggable.h"
#include "MailBox.h"
#include <boost/thread.hpp>
#include <boost/signal.hpp>

class CPsfVm : public CVirtualMachine, public boost::signals::trackable
{
public:
	typedef std::tr1::function<CSoundHandler* ()> SpuHandlerFactory;

						CPsfVm();
	virtual				~CPsfVm();

	void				Reset();
	void				Step();
	void				SetSpuHandler(const SpuHandlerFactory&);

	void				SetReverbEnabled(bool);
	void				SetVolumeAdjust(float);

	CMIPS&				GetCpu();
	Iop::CSpuBase&		GetSpuCore(unsigned int);
	uint8*				GetRam();
	void				SetSubSystem(const PsfVmSubSystemPtr&);

	CDebuggable			GetDebugInfo();

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
	void				ThreadProc();

	void				SetReverbEnabledImpl(bool);
	void				SetVolumeAdjustImpl(float);
	void				PauseImpl();
	void				SetSpuHandlerImpl(const SpuHandlerFactory&);

	STATUS				m_status;
	PsfVmSubSystemPtr	m_subSystem;
	CSoundHandler*		m_soundHandler;
	boost::thread*		m_thread;
	bool				m_singleStep;
	bool				m_isThreadOver;
	CMailBox			m_mailBox;
};

#endif
