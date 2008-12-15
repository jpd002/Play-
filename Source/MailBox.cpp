#include "MailBox.h"
#ifdef WIN32
#include "win32/Window.h"
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
	boost::mutex::scoped_lock waitLock(m_waitMutex);
    while(!IsPending())
    {
        m_waitCondition.wait(waitLock);
    }
}

void CMailBox::SendCall(const FunctionType& function, bool waitForCompletion)
{
    {
        boost::mutex::scoped_lock waitLock(m_waitMutex);
		MESSAGE message;
		message.function = function;
		message.sync = waitForCompletion;
        m_calls.push_back(message);
        m_waitCondition.notify_all();
    }
    if(waitForCompletion)
    {
        boost::mutex::scoped_lock doneLock(m_doneNotifyMutex);
        m_callDone = false;
		while(!m_callDone)
        {
#ifdef WIN32
	        MSG wmmsg;
            while(PeekMessage(&wmmsg, NULL, 0, 0, PM_REMOVE))
		    {
			    TranslateMessage(&wmmsg);
			    DispatchMessage(&wmmsg);
		    }
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 100 * 1000000;
            m_callFinished.timed_wait(doneLock, xt);
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
        boost::mutex::scoped_lock waitLock(m_waitMutex);
        if(!IsPending()) return;
        message = *m_calls.begin();
        m_calls.pop_front();
    }
    message.function();
	if(message.sync)
    {
        boost::mutex::scoped_lock doneLock(m_doneNotifyMutex);
        m_callDone = true;
        m_callFinished.notify_all();
    }
}
