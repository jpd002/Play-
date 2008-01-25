#ifndef _PADHANDLER_H_
#define _PADHANDLER_H_

#include "PadListener.h"
#include <list>
#include <functional>

class CPadHandler
{
public:
    typedef std::tr1::function<CPadHandler* (void)> FactoryFunction;

                                        CPadHandler();
    virtual								~CPadHandler();
    virtual void						Update(uint8*) = 0;
    void								InsertListener(CPadListener*);
    void								RemoveAllListeners();

protected:
    typedef std::list<CPadListener*>    ListenerList;
    ListenerList                        m_listeners;
};

#endif
