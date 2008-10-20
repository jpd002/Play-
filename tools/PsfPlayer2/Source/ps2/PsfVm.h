#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "VirtualMachine.h"
#include "PsfDevice.h"
#include "iop/IopBios.h"
#include "../Debuggable.h"

namespace PS2
{
    class CPsfVm : public CVirtualMachine
    {
    public:
                        CPsfVm();
        virtual         ~CPsfVm();

        void            LoadPsf(const CPsfDevice::PsfFile&);
        void            Reset();

        CDebuggable     GetIopDebug() const;

        virtual STATUS  GetStatus() const;
        virtual void    Pause();
        virtual void    Resume();

        virtual CMIPS&  GetCpu();

    private:
        uint8*          m_ram;
        CMIPS           m_iop;
        CIopBios        m_bios;
    };
}

#endif
