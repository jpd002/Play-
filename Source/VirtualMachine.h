#ifndef _VIRTUAL_MACHINE_H_
#define _VIRTUAL_MACHINE_H_

#include <boost/signals2.hpp>

class CVirtualMachine
{
public:
	enum STATUS
	{
		RUNNING = 1,
		PAUSED = 2,
	};

	virtual								~CVirtualMachine() {};
	virtual STATUS						GetStatus() const = 0;
	virtual void						Pause() = 0;
	virtual void						Resume() = 0;

	boost::signals2::signal<void ()>	OnMachineStateChange;
	boost::signals2::signal<void ()>	OnRunningStateChange;
};

#endif
