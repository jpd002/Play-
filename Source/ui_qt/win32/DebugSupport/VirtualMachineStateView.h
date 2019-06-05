#pragma once

#include "VirtualMachine.h"

class CVirtualMachineStateView
{
public:
	virtual ~CVirtualMachineStateView() = default;
	virtual void HandleMachineStateChange()
	{
	}
	virtual void HandleRunningStateChange(CVirtualMachine::STATUS)
	{
	}
};
