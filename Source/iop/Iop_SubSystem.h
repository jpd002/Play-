#ifndef _IOP_SUBSYSTEM_H_
#define _IOP_SUBSYSTEM_H_

#include "../MIPS.h"
#include "../MipsExecutor.h"
#include "Iop_SpuBase.h"
#include "Iop_Spu.h"
#include "Iop_Spu2.h"
#include "Iop_Dmac.h"
#include "Iop_Intc.h"
#include "Iop_RootCounters.h"
#include "Iop_BiosBase.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
    class CSubSystem
    {
    public:
                            CSubSystem();
        virtual             ~CSubSystem();

        void                Reset();
    	unsigned int		ExecuteCpu(bool);

        void                SetBios(CBiosBase*);

	    virtual void		SaveState(CZipArchiveWriter&);
	    virtual void		LoadState(CZipArchiveReader&);

	    uint8*				m_ram;
	    uint8*				m_scratchPad;
	    uint8*				m_spuRam;
	    CIntc			    m_intc;
	    CRootCounters	    m_counters;
	    CDmac			    m_dmac;
	    CSpuBase		    m_spuCore0;
	    CSpuBase		    m_spuCore1;
	    CSpu			    m_spu;
	    CSpu2			    m_spu2;
	    CMIPS				m_cpu;
	    CMipsExecutor		m_executor;
        CBiosBase*          m_bios;

    private:
	    enum
	    {
		    HW_REG_BEGIN	= 0x1F801000,
		    HW_REG_END		= 0x1F9FFFFF
	    };

    	uint32				ReadIoRegister(uint32);
    	uint32				WriteIoRegister(uint32, uint32);
    };
}

#endif
