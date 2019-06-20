#pragma once

#include "input/InputProvider.h"
#include <thread>
#include <Windows.h>
#include <Xinput.h>

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
		MAX_BUTTONS = 16
	};

	enum KEYID
	{
		KEYID_LTHUMB_X,
		KEYID_LTHUMB_Y,
		KEYID_RTHUMB_X,
		KEYID_RTHUMB_Y,
		KEYID_LTRIGGER,
		KEYID_RTRIGGER,
		KEYID_DPAD_UP,
		KEYID_DPAD_DOWN,
		KEYID_DPAD_LEFT,
		KEYID_DPAD_RIGHT,
		KEYID_START,
		KEYID_BACK,
		KEYID_LTHUMB,
		KEYID_RTHUMB,
		KEYID_LSHOULDER,
		KEYID_RSHOULDER,
		KEYID_A,
		KEYID_B,
		KEYID_X,
		KEYID_Y,
		KEYID_MAX
	};

	struct DEVICE_STATE
	{
		bool connected = false;
		uint32 idleTime = 0;
		bool lTriggerPressed = false;
		bool rTriggerPressed = false;
		uint16 buttonState = 0;
		uint32 packetNumber = 0;
	};

	void PollDevices();
	void UpdateConnectedDevice(uint32, const XINPUT_STATE&);
	void UpdateDisconnectedDevice(uint32);
	void ReportButton(uint32, KEYID, bool);
	void ReportAxis(uint32, KEYID, int32);

	bool m_pollingEnabled = true;
	std::thread m_pollingThread;

	static const char* g_keyNames[KEYID_MAX];
	static const KEYID g_buttonToKey[MAX_BUTTONS];
	DEVICE_STATE m_states[MAX_DEVICES];
};
