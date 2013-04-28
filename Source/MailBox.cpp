#include "MailBox.h"
#if defined(WIN32)
#include <Windows.h>
#endif

CMailBox::CMailBox()
{

}

CMailBox::~CMailBox()
{

}

bool CMailBox::IsPending() const
{
	return m_calls.size() != 0;
}

void CMailBox::WaitForCall()
{
	std::unique_lock<std::mutex> waitLock(m_waitMutex);
	while(!IsPending())
	{
		m_waitCondition.wait(waitLock);
	}
}

void CMailBox::WaitForCall(unsigned int timeOut)
{
	std::unique_lock<std::mutex> waitLock(m_waitMutex);
	if(IsPending()) return;
	m_waitCondition.wait_for(waitLock, std::chrono::milliseconds(timeOut));
}

void CMailBox::SendCall(const FunctionType& function, bool waitForCompletion)
{
	{
		std::unique_lock<std::mutex> waitLock(m_waitMutex);
		MESSAGE message;
		message.function = function;
		message.sync = waitForCompletion;
		m_calls.push_back(message);
		m_waitCondition.notify_all();
	}
	if(waitForCompletion)
	{
		std::unique_lock<std::mutex> doneLock(m_doneNotifyMutex);
		m_callDone = false;
		while(!m_callDone)
		{
#if defined(WIN32) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
			MSG wmmsg;
			while(PeekMessage(&wmmsg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&wmmsg);
				DispatchMessage(&wmmsg);
			}
			m_callFinished.wait_for(doneLock, std::chrono::milliseconds(100));
#else
			m_callFinished.wait(doneLock);
#endif
		}
	}
}

void CMailBox::ReceiveCall()
{
	MESSAGE message;
	{
		std::unique_lock<std::mutex> waitLock(m_waitMutex);
		if(!IsPending()) return;
		message = *m_calls.begin();
		m_calls.pop_front();
	}
	message.function();
	if(message.sync)
	{
		std::unique_lock<std::mutex> doneLock(m_doneNotifyMutex);
		m_callDone = true;
		m_callFinished.notify_all();
	}
}
