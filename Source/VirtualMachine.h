#ifndef _VIRTUAL_MACHINE_H_
#define _VIRTUAL_MACHINE_H_

#include <boost/signal.hpp>

class CVirtualMachine
{
public:
    enum STATUS
    {
        RUNNING = 1,
        PAUSED = 2,
    };

    virtual                     ~CVirtualMachine() {};
    virtual STATUS              GetStatus() const = 0;
    virtual void                Pause() = 0;
    virtual void                Resume() = 0;

	boost::signal<void ()>	    m_OnMachineStateChange;
	boost::signal<void ()>	    m_OnRunningStateChange;
};

#endif
