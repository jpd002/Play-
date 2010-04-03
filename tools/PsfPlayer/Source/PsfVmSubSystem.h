#ifndef _PSFVMSUBSYSTEM_H_
#define _PSFVMSUBSYSTEM_H_

#include <memory>
#include "MIPS.h"
#include "iop/Iop_SpuBase.h"
#include "SoundHandler.h"
#include <boost/signal.hpp>

class CPsfVmSubSystem
{
public:
	virtual					~CPsfVmSubSystem()	{}

	virtual void			Reset() = 0;
	virtual CMIPS&			GetCpu() = 0;
	virtual uint8*			GetRam() = 0;
	virtual Iop::CSpuBase&	GetSpuCore(unsigned int) = 0;

	virtual void			Update(bool, CSoundHandler*) = 0;

	boost::signal<void ()>	OnNewFrame;
};

typedef std::tr1::shared_ptr<CPsfVmSubSystem> PsfVmSubSystemPtr;

#endif
