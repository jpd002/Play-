#include "GamePadInputEventListener.h"
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <csignal>
#include <cstring>
#include <unistd.h>

CGamePadInputEventListener::CGamePadInputEventListener(std::string device)
    : m_device(device)
    , m_running(true)
{
	m_thread = std::thread([this]() { InputDeviceListenerThread(); });
}

CGamePadInputEventListener::~CGamePadInputEventListener()
{
	m_running = false;
	OnInputEvent.Reset();
	m_thread.join();
}

void CGamePadInputEventListener::InputDeviceListenerThread()
{
	if(access(m_device.c_str(), R_OK) == -1)
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

	struct libevdev* dev = NULL;
	int initdev_result = libevdev_new_from_fd(fd, &dev);
	if(initdev_result < 0)
	{
		fprintf(stderr, "CGamePadInputEventListener::InputDeviceListenerThread Failed to init libevdev (%s)\n", strerror(-initdev_result));
		libevdev_free(dev);
		close(fd);
		return;
	}
	initdev_result = 0;

	auto device = CGamePadUtils::GetDeviceID(dev);

	struct timespec ts;
	ts.tv_nsec = 5e+8; // 500 millisecond

	fd_set fds;

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	while(m_running)
	{
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		if(pselect(fd + 1, &fds, NULL, NULL, &ts, &mask) == 0) continue;

		int rc = 0;
		do
		{
			struct input_event ev;
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
			if(rc == LIBEVDEV_READ_STATUS_SYNC)
			{
				while(rc == LIBEVDEV_READ_STATUS_SYNC)
				{
					rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
				}
			}
			else if(rc == LIBEVDEV_READ_STATUS_SUCCESS && ev.type != EV_SYN)
			{
				const struct input_absinfo* abs = nullptr;
				if(ev.type == EV_ABS) abs = libevdev_get_abs_info(dev, ev.code);
				OnInputEvent(device, ev.code, ev.value, ev.type, abs);
			}
		} while(rc != -EAGAIN && m_running);
	}

	libevdev_free(dev);
	close(fd);
}
