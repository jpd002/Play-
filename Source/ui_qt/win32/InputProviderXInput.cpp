#include "InputProviderXInput.h"
#include <Windows.h>
#include <Xinput.h>

#define PROVIDER_ID 'xinp'

CInputProviderXInput::CInputProviderXInput()
{
	m_pollingThread = std::thread([this] () { PollDevices(); });
}

CInputProviderXInput::~CInputProviderXInput()
{
	m_pollingEnabled = false;
	m_pollingThread.join();
}

uint32 CInputProviderXInput::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderXInput::GetTargetDescription(const BINDINGTARGET& target) const
{
	return "XInputTarget";
}

void CInputProviderXInput::PollDevices()
{
	while(m_pollingEnabled)
	{
		bool hasActiveDevice = false;
		for(unsigned int i = 0; i < MAX_DEVICES; i++)
		{
			auto& prevState = m_states[i];
			XINPUT_STATE state;
			DWORD result = XInputGetState(i, &state);
			if(result == ERROR_SUCCESS)
			{
				prevState.connected = true;
				hasActiveDevice = true;
			}
			else
			{
				prevState.connected = false;
			}
		}
		Sleep(hasActiveDevice ? 16 : 1000);
	}
}
