#pragma once

#include <memory>
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"
#include "iop/Iop_SpuBase.h"
#include "SoundHandler.h"
#include <boost/signals2.hpp>

class CPsfVmSubSystem
{
public:
	virtual								~CPsfVmSubSystem() = default;

	virtual void						Reset() = 0;
	virtual CMIPS&						GetCpu() = 0;
	virtual uint8*						GetRam() = 0;
	virtual uint8*						GetSpr() = 0;
	virtual Iop::CSpuBase&				GetSpuCore(unsigned int) = 0;

	virtual void						Update(bool, CSoundHandler*) = 0;

#ifdef DEBUGGER_INCLUDED
	virtual bool						MustBreak() = 0;
	virtual void						DisableBreakpointsOnce() = 0;
	virtual CBiosDebugInfoProvider*		GetBiosDebugInfoProvider() = 0;
	virtual void						LoadDebugTags(Framework::Xml::CNode*) = 0;
	virtual void						SaveDebugTags(Framework::Xml::CNode*) = 0;
#endif

	boost::signals2::signal<void ()>	OnNewFrame;
};

typedef std::shared_ptr<CPsfVmSubSystem> PsfVmSubSystemPtr;
