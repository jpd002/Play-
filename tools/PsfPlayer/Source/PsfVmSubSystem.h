#ifndef _PSFVMSUBSYSTEM_H_
#define _PSFVMSUBSYSTEM_H_

#include <memory>
#include "MIPS.h"
#include "MipsModule.h"
#include "iop/Iop_SpuBase.h"
#include "SoundHandler.h"
#include <boost/signal.hpp>

class CPsfVmSubSystem
{
public:
	virtual								~CPsfVmSubSystem()	{}

	virtual void						Reset() = 0;
	virtual CMIPS&						GetCpu() = 0;
	virtual uint8*						GetRam() = 0;
	virtual Iop::CSpuBase&				GetSpuCore(unsigned int) = 0;

	virtual void						Update(bool, CSoundHandler*) = 0;

#ifdef DEBUGGER_INCLUDED
	virtual bool						MustBreak() = 0;
	virtual MipsModuleList				GetModuleList() = 0;
	virtual void						LoadDebugTags(Framework::Xml::CNode*) = 0;
	virtual void						SaveDebugTags(Framework::Xml::CNode*) = 0;
#endif

	boost::signal<void ()>				OnNewFrame;
};

typedef std::tr1::shared_ptr<CPsfVmSubSystem> PsfVmSubSystemPtr;

#endif
