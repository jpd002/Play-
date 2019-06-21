#include "InputProviderXInput.h"
#include "string_format.h"

#define PROVIDER_ID 'xinp'

// clang-format off
const char* CInputProviderXInput::g_keyNames[CInputProviderXInput::KEYID_MAX] =
{
	"Left Thumb (X Axis)",
	"Left Thumb (Y Axis)",
	"Right Thumb (X Axis)",
	"Right Thumb (Y Axis)",
	"Left Trigger",
	"Right Trigger",
	"DPad Up",
	"DPad Down",
	"DPad Left",
	"DPad Right",
	"Start",
	"Back",
	"Left Thumb",
	"Right Thumb",
	"Left Shoulder",
	"Right Shoulder",
	"A",
	"B",
	"X",
	"Y"
};

const CInputProviderXInput::KEYID CInputProviderXInput::g_buttonToKey[MAX_BUTTONS] =
{
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
	KEYID_MAX,
	KEYID_MAX,
	KEYID_A,
	KEYID_B,
	KEYID_X,
	KEYID_Y
};
// clang-format on

CInputProviderXInput::CInputProviderXInput()
{
	m_pollingThread = std::thread([this]() { PollDevices(); });
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
	return string_format("XInput Device %d : %s", target.deviceId[0], g_keyNames[target.keyId]);
}

void CInputProviderXInput::PollDevices()
{
	while(m_pollingEnabled)
	{
		for(unsigned int deviceIndex = 0; deviceIndex < MAX_DEVICES; deviceIndex++)
		{
			auto& currState = m_states[deviceIndex];
			if(currState.idleTime > 0)
			{
				currState.idleTime--;
				continue;
			}
			XINPUT_STATE state;
			DWORD result = XInputGetState(deviceIndex, &state);
			if(result == ERROR_SUCCESS)
			{
				UpdateConnectedDevice(deviceIndex, state);
			}
			else
			{
				UpdateDisconnectedDevice(deviceIndex);
			}
		}
		Sleep(16);
	}
}

void CInputProviderXInput::UpdateConnectedDevice(uint32 deviceIndex, const XINPUT_STATE& state)
{
	auto& currState = m_states[deviceIndex];
	if((currState.connected == false) || (currState.packetNumber != state.dwPacketNumber))
	{
		ReportAxis(deviceIndex, KEYID_LTHUMB_X, state.Gamepad.sThumbLX);
		ReportAxis(deviceIndex, KEYID_LTHUMB_Y, -static_cast<int32>(state.Gamepad.sThumbLY));
		ReportAxis(deviceIndex, KEYID_RTHUMB_X, state.Gamepad.sThumbRX);
		ReportAxis(deviceIndex, KEYID_RTHUMB_Y, -static_cast<int32>(state.Gamepad.sThumbRY));
		bool lTriggerState = state.Gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		bool rTriggerState = state.Gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		if(lTriggerState != currState.lTriggerPressed)
		{
			ReportButton(deviceIndex, KEYID_LTRIGGER, lTriggerState);
			currState.lTriggerPressed = lTriggerState;
		}
		if(rTriggerState != currState.rTriggerPressed)
		{
			ReportButton(deviceIndex, KEYID_RTRIGGER, rTriggerState);
			currState.rTriggerPressed = rTriggerState;
		}
		for(unsigned int keyIndex = 0; keyIndex < MAX_BUTTONS; keyIndex++)
		{
			auto keyId = g_buttonToKey[keyIndex];
			if(keyId == KEYID_MAX) continue;
			bool newState = (state.Gamepad.wButtons & (1 << keyIndex)) != 0;
			bool oldState = (currState.buttonState & (1 << keyIndex));
			if(newState != oldState)
			{
				ReportButton(deviceIndex, keyId, newState);
			}
		}
		currState.buttonState = state.Gamepad.wButtons;
		currState.packetNumber = state.dwPacketNumber;
	}
	currState.connected = true;
}

void CInputProviderXInput::UpdateDisconnectedDevice(uint32 deviceIndex)
{
	auto& currState = m_states[deviceIndex];
	if(currState.connected)
	{
		//Going from connected to disconnected, reset state
		ReportAxis(deviceIndex, KEYID_LTHUMB_X, BINDINGTARGET::AXIS_NEUTRAL);
		ReportAxis(deviceIndex, KEYID_LTHUMB_Y, BINDINGTARGET::AXIS_NEUTRAL);
		ReportAxis(deviceIndex, KEYID_RTHUMB_X, BINDINGTARGET::AXIS_NEUTRAL);
		ReportAxis(deviceIndex, KEYID_RTHUMB_Y, BINDINGTARGET::AXIS_NEUTRAL);
		ReportButton(deviceIndex, KEYID_LTRIGGER, false);
		ReportButton(deviceIndex, KEYID_RTRIGGER, false);
		for(unsigned int keyIndex = 0; keyIndex < MAX_BUTTONS; keyIndex++)
		{
			auto keyId = g_buttonToKey[keyIndex];
			if(keyId == KEYID_MAX) continue;
			ReportButton(deviceIndex, keyId, false);
		}
		currState.lTriggerPressed = false;
		currState.rTriggerPressed = false;
		currState.buttonState = 0;
	}

	currState.idleTime = 100;
	currState.connected = false;
}

void CInputProviderXInput::ReportButton(uint32 deviceIndex, KEYID keyId, bool pressed)
{
	BINDINGTARGET tgt;
	tgt.providerId = PROVIDER_ID;
	tgt.deviceId[0] = deviceIndex;
	tgt.keyType = BINDINGTARGET::KEYTYPE::BUTTON;
	tgt.keyId = keyId;
	OnInput(tgt, pressed ? 1 : 0);
}

void CInputProviderXInput::ReportAxis(uint32 deviceIndex, KEYID keyId, int32 rawValue)
{
	BINDINGTARGET tgt;
	tgt.providerId = PROVIDER_ID;
	tgt.deviceId[0] = deviceIndex;
	tgt.keyType = BINDINGTARGET::KEYTYPE::AXIS;
	tgt.keyId = keyId;
	uint32 cvtValue = std::min<uint32>((rawValue + 0x8000) >> 8, BINDINGTARGET::AXIS_MAX);
	OnInput(tgt, cvtValue);
}
