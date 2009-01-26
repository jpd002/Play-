#ifndef _IOP_BIOSBASE_H_
#define _IOP_BIOSBASE_H_

#include "Types.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#ifdef DEBUGGER_INCLUDED
#include "xml/Node.h"
#include "../MipsModule.h"
#endif

namespace Iop
{
    class CBiosBase
    {
    public:
        virtual						~CBiosBase() {}
        virtual void				HandleException() = 0;
        virtual void				HandleInterrupt() = 0;
	    virtual void				CountTicks(uint32) = 0;

        virtual bool                IsIdle() = 0;

	    virtual void		        SaveState(CZipArchiveWriter&) = 0;
	    virtual void		        LoadState(CZipArchiveReader&) = 0;

#ifdef DEBUGGER_INCLUDED
		virtual void				SaveDebugTags(Framework::Xml::CNode*) = 0;
		virtual void				LoadDebugTags(Framework::Xml::CNode*) = 0;
		virtual MipsModuleList		GetModuleList() = 0;
#endif

    private:

    };
}

#endif
