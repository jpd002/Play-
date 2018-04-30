#include "GamePadDeviceListener.h"
#include "GamePadUtils.h"
#include <fcntl.h>
#include <sys/signalfd.h>
#include <poll.h>
#include <sys/inotify.h>
#include <csignal>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))
#define WATCH_FLAGS (IN_CREATE | IN_DELETE)

CGamePadDeviceListener::CGamePadDeviceListener(OnInputEvent OnInputEventCallBack, bool filter)
    : OnInputEventCallBack(OnInputEventCallBack)
    , m_running(true)
    , m_filter(filter)
{
	m_thread = std::thread([this]() { InputDeviceListenerThread(); });
	UpdateDeviceList();
}

CGamePadDeviceListener::CGamePadDeviceListener(bool filter)
    : CGamePadDeviceListener(nullptr, filter)
{
}

CGamePadDeviceListener::~CGamePadDeviceListener()
{
	m_running = false;
	m_GPIEList.clear();
	m_thread.join();
}

void CGamePadDeviceListener::RePopulateAbs()
{
	for(auto& GPI : m_GPIEList)
	{
		GPI.second.get()->RePopulateAbs();
	}
}

void CGamePadDeviceListener::UpdateOnInputEventCallback(CGamePadDeviceListener::OnInputEvent OnInputEventFunction)
{
	OnInputEventCallBack = OnInputEventFunction;
	for(auto& device : m_devicelist)
	{
		auto GPI = m_GPIEList.find(device.first);
		if(GPI != m_GPIEList.end())
		{
			GPI->second.get()->OnInputEvent.disconnect_all_slots();
			GPI->second.get()->OnInputEvent.connect(OnInputEventCallBack);
		}
		else
		{
			auto GamePadInput = std::make_unique<CGamePadInputEventListener>(device.second.path, m_filter);
			GamePadInput->OnInputEvent.connect(OnInputEventCallBack);
			m_GPIEList.emplace(device.first, std::move(GamePadInput));
		}
	}
}

void CGamePadDeviceListener::DisconnectInputEventCallback()
{
	OnInputEventCallBack = nullptr;
	for(auto& GPI : m_GPIEList)
	{
		GPI.second.get()->OnInputEvent.disconnect_all_slots();
	}
}

void CGamePadDeviceListener::UpdateDeviceList()
{
	std::string path = "/dev/input/";
	for(auto& p : fs::directory_iterator(path))
	{
		if(p.path().filename().string().find("event") != std::string::npos)
		{
			AddDevice(p.path());
		}
	}
}

void CGamePadDeviceListener::AddDevice(const fs::path& path)
{
	inputdev_pair idp;
	if(IsValidDevice(path, idp))
	{
		m_devicelist.insert(idp);
		if(OnInputEventCallBack)
		{
			auto GamePadInput = std::make_unique<CGamePadInputEventListener>(idp.second.path, m_filter);
			GamePadInput->OnInputEvent.connect(OnInputEventCallBack);
			m_GPIEList.emplace(idp.first, std::move(GamePadInput));
		}
	}
}

void CGamePadDeviceListener::RemoveDevice(std::string key)
{
	m_devicelist.erase(key);
	m_GPIEList.erase(key);
}

void CGamePadDeviceListener::InputDeviceListenerThread()
{
	int fd = inotify_init1(IN_NONBLOCK);
	if(fd < 0)
	{
		perror("inotify_init");
		return;
	}

	struct pollfd fds[2];
	sigset_t mask;

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	fds[1].fd = signalfd(-1, &mask, SFD_NONBLOCK);
	fds[1].events = POLLIN;

	sigprocmask(SIG_BLOCK, &mask, NULL);

	std::string devinput_dir("/dev/input");
	int wd = inotify_add_watch(fd, devinput_dir.c_str(), WATCH_FLAGS);
	char buffer[EVENT_BUF_LEN];

	while(m_running)
	{
		if(poll(fds, 2, 500) == 0) continue;

		int length = read(fd, buffer, EVENT_BUF_LEN);
		if(length < 0)
		{
			perror("read");
			continue;
		}

		for(int i = 0; i < length;)
		{
			struct inotify_event* event = (struct inotify_event*)&buffer[i];
			if(event->len)
			{
				if(strstr(event->name, "event"))
				{
					if(event->mask & IN_CREATE)
					{
						//Allow the device to be initiated
						std::this_thread::sleep_for(std::chrono::seconds(1));
						AddDevice(devinput_dir + "/" + event->name);
					}
					else if(event->mask & IN_DELETE)
					{
						RemoveDevice(event->name);
					}
				}
			}
			i += EVENT_SIZE + event->len;
		}
	}

	inotify_rm_watch(fd, wd);
	close(fd);
}

bool CGamePadDeviceListener::IsValidDevice(const fs::path& inputdev_path, inputdev_pair& devinfo)
{
	if(access(inputdev_path.string().c_str(), R_OK) == -1)
	{
		return false;
	}
	bool res = false;

	int fd = open(inputdev_path.string().c_str(), O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "CGamePadDeviceListener::IsValidDevice: Error Failed to open (%s)\n", inputdev_path.string().c_str());
		return res;
	}

	struct libevdev* dev = NULL;
	int initdev_result = libevdev_new_from_fd(fd, &dev);
	if(initdev_result < 0)
	{
		fprintf(stderr, "CGamePadDeviceListener::IsValidDevice: Failed to init libevdev (%s)\n", strerror(-initdev_result));
		libevdev_free(dev);
		close(fd);
		return res;
	}

	auto device = CGamePadUtils::GetDeviceID(dev);

	std::string name;
	if(libevdev_get_name(dev) != NULL)
	{
		name = std::string(libevdev_get_name(dev));
	}

	CGamePadDeviceListener::inputdevice id;
	id.name = name;
	id.uniq_id = device;
	id.path = inputdev_path.string();
	;
	devinfo = std::make_pair(inputdev_path.filename().string(), id);
	res = true;

	libevdev_free(dev);
	close(fd);
	return res;
}
