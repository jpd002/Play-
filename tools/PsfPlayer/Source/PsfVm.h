#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "Types.h"
#include "MIPS.h"
#include "SoundHandler.h"
#include "iop/Iop_SubSystem.h"
#include "VirtualMachine.h"
#include "Debuggable.h"
#include "MailBox.h"
#include "MipsExecutor.h"
#include <boost/thread.hpp>
#include <boost/signal.hpp>

class CPsfVm : public CVirtualMachine
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
    uint8*              GetRam();

    void                SetBios(const Iop::CSubSystem::BiosPtr&);

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
	MipsModuleList		GetIopModules();
	void				ThreadProc();

	void				SetReverbEnabledImpl(bool);
	void				SetVolumeAdjustImpl(float);
	void				PauseImpl();
	void				SetSpuHandlerImpl(const SpuHandlerFactory&);

	STATUS				m_status;
	Iop::CSubSystem		m_iop;
	CSoundHandler*		m_spuHandler;
	boost::thread*		m_thread;
	bool				m_singleStep;
    int                 m_spuUpdateCounter;
    int                 m_frameCounter;
    bool                m_isThreadOver;
	CMailBox			m_mailBox;
};

#endif
