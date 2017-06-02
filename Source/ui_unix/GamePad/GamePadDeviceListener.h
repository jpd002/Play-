#pragma once
#include <atomic>
#include <boost/signals2.hpp>
#include <thread>
#include <libevdev.h>
#include <boost/filesystem.hpp>

#include "GamePadInputEventListener.h"
#include "Types.h"

namespace fs = boost::filesystem;

class CGamePadDeviceListener
{
public:
	typedef std::function<void(std::array<uint32, 6>, int, int, int, const input_absinfo*)> OnInputEvent;

	CGamePadDeviceListener(bool f = false);
	CGamePadDeviceListener(OnInputEvent, bool f = false);

	~CGamePadDeviceListener();

	struct inputdevice
	{
		std::string name;
		std::array<uint32, 6> uniq_id;
		std::string path;
	};
	typedef std::pair <std::string, CGamePadDeviceListener::inputdevice>	inputdev_pair;
	OnInputEvent			OnInputEventCallBack;

	void					UpdateOnInputEventCallback(OnInputEvent);
	void					DisconnectInputEventCallback();
	void					RePopulateAbs();
	static bool				IsValidDevice(const fs::path&, inputdev_pair&);
private:
	std::map<std::string, CGamePadDeviceListener::inputdevice>		m_devicelist;
	std::map<std::string, std::unique_ptr<CGamePadInputEventListener>>	m_GPIEList;
	std::atomic<bool>		m_running;
	std::thread				m_inputdevicelistenerthread;
	bool					m_filter;
	std::thread				m_thread;

	void					UpdateDeviceList();
	void					AddDevice(const fs::path&);
	void					RemoveDevice(std::string);
	void					InputDeviceListenerThread();
};
