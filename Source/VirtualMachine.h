#pragma once

#include "Signal.h"

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

	Framework::CSignal<void()> OnMachineStateChange;
	Framework::CSignal<void()> OnRunningStateChange;
};
