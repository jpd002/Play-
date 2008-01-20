#ifndef _SIFMODULEADAPTER_H_
#define _SIFMODULEADAPTER_H_

#include <functional>
#include "SifModule.h"

class CSifModuleAdapter : public CSifModule
{
public:
    typedef std::tr1::function<void (uint32, uint32*, uint32, uint32*, uint32, uint8*)> SifCommandHandler;

    CSifModuleAdapter()
    {

    }

    CSifModuleAdapter(const SifCommandHandler& handler) : 
    m_handler(handler) 
    {

    }
    virtual ~CSifModuleAdapter() 
    {
    
    }

    virtual void Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
    { 
        m_handler(method, args, argsSize, ret, retSize, ram);
    }

private:
    SifCommandHandler m_handler;
};

#endif
