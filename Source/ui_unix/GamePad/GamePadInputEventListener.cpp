#include "GamePadInputEventListener.h"
#include "GamePadUtils.h"
#include <fcntl.h>
#include <sys/signalfd.h>
#include <poll.h>
#include <csignal>

CGamePadInputEventListener::CGamePadInputEventListener(std::string device, bool filter) :
	m_device(device),
	m_filter(filter),
	m_running(true)
{
	m_thread = std::thread([this](){InputDeviceListenerThread();});
}

CGamePadInputEventListener::~CGamePadInputEventListener()
{
	m_running = false;
	OnInputEvent.disconnect_all_slots();
	m_thread.join();
}

void CGamePadInputEventListener::PopulateAbsInfoList(libevdev *dev)
{
	if(libevdev_has_event_type(dev, EV_ABS))
	{
		for(int i = 0; i <= ABS_MAX; i++)
		{
			if(libevdev_has_event_code(dev, EV_ABS, i))
			{
				m_abslist.at(i) = *libevdev_get_abs_info(dev, i);
			}
		}
	}
}

void CGamePadInputEventListener::RePopulateAbs()
{
	if(m_filter)
	{
		if(access( m_device.c_str(), R_OK ) == -1)
		{
			fprintf(stderr, "CGamePadInputEventListener::RePopulateAbs: no read access to (%s)\n", m_device.c_str());
			return;
		}
		struct libevdev *dev = NULL;
		int fd = open(m_device.c_str(), O_RDONLY);
		if(fd < 0)
		{
			perror("CGamePadInputEventListener::RePopulateAbs Failed to open device");
		}
		else
		{
			int rc = libevdev_new_from_fd(fd, &dev);
			if(rc < 0)
			{
				fprintf(stderr, "CGamePadInputEventListener::RePopulateAbs Failed to init libevdev (%s)\n", strerror(-rc));
			}
			else
			{
				PopulateAbsInfoList(dev);
			}
			libevdev_free(dev);
			close(fd);
		}
	}
}
void CGamePadInputEventListener::InputDeviceListenerThread()
{
	if(access( m_device.c_str(), R_OK ) == -1)
	{
		fprintf(stderr, "CGamePadInputEventListener::InputDeviceListenerThread: no read access to (%s)\n", m_device.c_str());
		return;
	}

	int fd = open(m_device.c_str(), O_RDONLY | O_NONBLOCK);
	if(fd < 0)
	{
		perror("CGamePadInputEventListener::InputDeviceListenerThread Failed to open device");
		return;
	}

	struct libevdev *dev = NULL;
	int initdev_result = libevdev_new_from_fd(fd, &dev);
	if(initdev_result < 0)
	{
		fprintf(stderr, "CGamePadInputEventListener::InputDeviceListenerThread Failed to init libevdev (%s)\n", strerror(-initdev_result));
		libevdev_free(dev);
		close(fd);
	}
	initdev_result = 0;

	if(m_filter)
	{
		PopulateAbsInfoList(dev);
	}

	auto device = CGamePadUtils::GetDeviceID(dev);

	struct pollfd fds[2];
	sigset_t mask;

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	fds[1].fd = signalfd(-1, &mask, SFD_NONBLOCK);
	fds[1].events = POLLIN;
	sigprocmask(SIG_BLOCK, &mask, NULL);

	while(m_running)
	{
		if(poll(fds, 2, 500) == 0) continue;

		int rc = 0;
		do {
			struct input_event ev;
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
			if(rc == LIBEVDEV_READ_STATUS_SYNC)
			{
				while (rc == LIBEVDEV_READ_STATUS_SYNC)
				{
					rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
				}
			}
			else if(rc == LIBEVDEV_READ_STATUS_SUCCESS && ev.type != EV_SYN)
			{

				if(m_filter && ev.type == EV_ABS)
				{
					int range = m_abslist.at(ev.code).maximum/100*20;
					if(ev.value < m_abslist.at(ev.code).value + range && ev.value > m_abslist.at(ev.code).value - range)
					{
						continue;
					}
				}
				const struct input_absinfo *abs;
				if(ev.type == 3) abs = libevdev_get_abs_info(dev, ev.code);
				if(m_running)
				{
					OnInputEvent(device, ev.code, ev.value, ev.type, abs);
				}
			}
		} while (rc != -EAGAIN && m_running);
	}

	libevdev_free(dev);
	close(fd);
}
