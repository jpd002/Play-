#ifndef _PSFVM_H_
#define _PSFVM_H_

#include <boost/thread.hpp>
#include <boost/signal.hpp>
#include "VirtualMachine.h"
#include "PsfDevice.h"
#include "iop/IopBios.h"
#include "../Debuggable.h"
#include "MipsExecutor.h"
#include "MailBox.h"
#include "Spu2.h"
#include "iop/Iop_Intc.h"
#include "Iop_Dmac.h"

namespace PS2
{
    class CPsfVm : public CVirtualMachine
    {
    public:
								CPsfVm();
        virtual					~CPsfVm();

        void					LoadPsf(const CPsfDevice::PsfFile&);
        void					Reset();

        CDebuggable				GetIopDebug();

        virtual STATUS			GetStatus() const;
        virtual void			Pause();
        virtual void			Resume();

		void					Step();
        CMIPS&					GetCpu();

		boost::signal<void ()>	OnNewFrame;

	private:
		unsigned int			ExecuteCpu(bool);
		void					ThreadProc();

		uint8*					m_ram;
        CSpu2					m_spu;
        Iop::CDmac              m_dmac;
        Iop::CIntc              m_intc;
		CMipsExecutor			m_executor;
        CMIPS					m_cpu;
        CIopBios				m_bios;
		CMailBox				m_mailBox;
		STATUS					m_status;
		boost::thread			m_thread;
		bool					m_singleStep;
    };
}

#endif
