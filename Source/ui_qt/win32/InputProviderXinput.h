#pragma once

#include "input/InputProvider.h"
#include <thread>

class CInputProviderXInput : public CInputProvider
{
public:
	CInputProviderXInput();
	virtual ~CInputProviderXInput();

	uint32 GetId() const override;
	std::string GetTargetDescription(const BINDINGTARGET&) const override;

private:
	enum
	{
		MAX_DEVICES = 4,
	};

	struct DEVICE_STATE
	{
		bool connected = false;
	};

	void PollDevices();

	bool m_pollingEnabled = true;
	std::thread m_pollingThread;

	DEVICE_STATE m_states[MAX_DEVICES];
};
