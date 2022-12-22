#include <cstring>
#include <climits>
#include "GamePadDeviceListener.h"
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <sys/inotify.h>
#include <csignal>
#include <unistd.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))
#define WATCH_FLAGS (IN_CREATE | IN_DELETE)

CGamePadDeviceListener::CGamePadDeviceListener(OnInputEvent OnInputEventCallBack)
    : OnInputEventCallBack(OnInputEventCallBack)
    , m_running(true)
{
	m_thread = std::thread([this]() { InputDeviceListenerThread(); });
	UpdateDeviceList();
}

CGamePadDeviceListener::~CGamePadDeviceListener()
{
	m_running = false;
	m_GPIEList.clear();
	m_thread.join();
}

void CGamePadDeviceListener::UpdateOnInputEventCallback(CGamePadDeviceListener::OnInputEvent OnInputEventFunction)
{
	m_connectionlist.clear();
	OnInputEventCallBack = OnInputEventFunction;
	for(auto& device : m_devicelist)
	{
		auto GPI = m_GPIEList.find(device.first);
		if(GPI != m_GPIEList.end())
		{
			m_connectionlist.emplace(GPI->first, GPI->second.get()->OnInputEvent.Connect(OnInputEventCallBack));
		}
		else
		{
			auto GamePadInput = std::make_unique<CGamePadInputEventListener>(device.second.path);
			m_connectionlist.emplace(device.first, GamePadInput->OnInputEvent.Connect(OnInputEventCallBack));
			m_GPIEList.emplace(device.first, std::move(GamePadInput));
		}
	}
}

void CGamePadDeviceListener::DisconnectInputEventCallback()
{
	OnInputEventCallBack = nullptr;
	m_connectionlist.clear();
}

void CGamePadDeviceListener::UpdateDeviceList()
{
	auto path = fs::path("/dev/input/");
	if(!fs::exists(path)) return;
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
			auto GamePadInput = std::make_unique<CGamePadInputEventListener>(idp.second.path);
			m_connectionlist.emplace(idp.first, GamePadInput->OnInputEvent.Connect(OnInputEventCallBack));
			m_GPIEList.emplace(idp.first, std::move(GamePadInput));
		}
	}
}

void CGamePadDeviceListener::RemoveDevice(std::string key)
{
	m_devicelist.erase(key);
	m_GPIEList.erase(key);
	m_connectionlist.erase(key);
}

void CGamePadDeviceListener::InputDeviceListenerThread()
{
	int fd = inotify_init1(IN_NONBLOCK);
	if(fd < 0)
	{
		perror("inotify_init");
		return;
	}

	struct timespec ts;
	ts.tv_nsec = 5e+8; // 500 millisecond

	fd_set fds;

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	std::string devinput_dir("/dev/input");
	int wd = inotify_add_watch(fd, devinput_dir.c_str(), WATCH_FLAGS);
	char buffer[EVENT_BUF_LEN];

	while(m_running)
	{
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		if(pselect(fd + 1, &fds, NULL, NULL, &ts, &mask) == 0) continue;

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

	devinfo = std::make_pair(inputdev_path.filename().string(), id);
	res = true;

	libevdev_free(dev);
	close(fd);
	return res;
}
