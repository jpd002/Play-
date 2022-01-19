#pragma once

#include "signal/Signal.h"

class CVirtualMachine
{
public:
	enum STATUS
	{
		RUNNING = 1,
		PAUSED = 2,
	};

	virtual ~CVirtualMachine() = default;
	virtual STATUS GetStatus() const = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;

	using MachineStateChangeEvent = Framework::CSignal<void()>;
	MachineStateChangeEvent OnMachineStateChange;
	using RunningStateChangeEvent = Framework::CSignal<void()>;
	RunningStateChangeEvent OnRunningStateChange;
};
