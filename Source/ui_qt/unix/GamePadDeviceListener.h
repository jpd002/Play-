#pragma once

#include <atomic>
#include <thread>
#include <map>
#include <libevdev.h>

#include "GamePadUtils.h"
#include "GamePadInputEventListener.h"
#include "Types.h"

#include "filesystem_def.h"

class CGamePadDeviceListener
{
public:
	typedef std::function<void(GamePadDeviceId, int, int, int, const input_absinfo*)> OnInputEvent;

	CGamePadDeviceListener(OnInputEvent);

	~CGamePadDeviceListener();

	struct inputdevice
	{
		std::string name;
		std::array<uint32, 6> uniq_id;
		std::string path;
	};
	typedef std::pair<std::string, CGamePadDeviceListener::inputdevice> inputdev_pair;
	OnInputEvent OnInputEventCallBack;

	void UpdateOnInputEventCallback(OnInputEvent);
	void DisconnectInputEventCallback();
	static bool IsValidDevice(const fs::path&, inputdev_pair&);

private:
	std::map<std::string, CGamePadDeviceListener::inputdevice> m_devicelist;
	std::map<std::string, std::unique_ptr<CGamePadInputEventListener>> m_GPIEList;
	std::atomic<bool> m_running;
	std::thread m_inputdevicelistenerthread;
	std::thread m_thread;
	std::map<std::string, CGamePadInputEventListener::OnInputEventType::Connection> m_connectionlist;

	void UpdateDeviceList();
	void AddDevice(const fs::path&);
	void RemoveDevice(std::string);
	void InputDeviceListenerThread();
};
