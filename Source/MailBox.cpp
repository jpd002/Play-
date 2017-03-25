#include "MailBox.h"
#if defined(_WIN32)
#include "win32/Win32Defs.h"
#endif

bool CMailBox::IsPending() const
{
	return m_calls.size() != 0;
}

void CMailBox::WaitForCall()
{
	std::unique_lock<std::mutex> callLock(m_callMutex);
	while(!IsPending())
	{
		m_waitCondition.wait(callLock);
	}
}

void CMailBox::WaitForCall(unsigned int timeOut)
{
	std::unique_lock<std::mutex> callLock(m_callMutex);
	if(IsPending()) return;
	m_waitCondition.wait_for(callLock, std::chrono::milliseconds(timeOut));
}

void CMailBox::FlushCalls()
{
	SendCall([] () { }, true);
}

void CMailBox::SendCall(const FunctionType& function, bool waitForCompletion)
{
	std::unique_lock<std::mutex> callLock(m_callMutex);

	{
		MESSAGE message;
		message.function = function;
		message.sync     = waitForCompletion;
		m_calls.push_back(std::move(message));
	}
	
	m_waitCondition.notify_all();

	if(waitForCompletion)
	{
		m_callDone = false;
		while(!m_callDone)
		{
#ifdef FRAMEWORK_WIN_DESKTOP
			MSG wmmsg;
			while(PeekMessage(&wmmsg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&wmmsg);
				DispatchMessage(&wmmsg);
			}
			m_callFinished.wait_for(callLock, std::chrono::milliseconds(100));
#else
			m_callFinished.wait(callLock);
#endif
		}
	}
}

void CMailBox::SendCall(FunctionType&& function)
{
	std::lock_guard<std::mutex> callLock(m_callMutex);
	
	{
		MESSAGE message;
		message.function = std::move(function);
		message.sync     = false;
		m_calls.push_back(std::move(message));
	}
	
	m_waitCondition.notify_all();
}

void CMailBox::ReceiveCall()
{
	MESSAGE message;
	{
		std::lock_guard<std::mutex> waitLock(m_callMutex);
		if(!IsPending()) return;
		message = std::move(m_calls.front());
		m_calls.pop_front();
	}
	message.function();
	if(message.sync)
	{
		std::lock_guard<std::mutex> waitLock(m_callMutex);
		m_callDone = true;
		m_callFinished.notify_all();
	}
}
