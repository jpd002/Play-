#ifndef _IOP_BIOSBASE_H_
#define _IOP_BIOSBASE_H_

#include "Types.h"
#ifdef DEBUGGER_INCLUDED
#include "xml/Node.h"
#endif

namespace Iop
{
    class CBiosBase
    {
    public:
        virtual         ~CBiosBase() {}
        virtual void    HandleException() = 0;
        virtual void    HandleInterrupt() = 0;
	    virtual void	CountTicks(uint32) = 0;

#ifdef DEBUGGER_INCLUDED
		virtual void	SaveDebugTags(Framework::Xml::CNode*) = 0;
		virtual void	LoadDebugTags(Framework::Xml::CNode*) = 0;
#endif

    private:

    };
}

#endif
