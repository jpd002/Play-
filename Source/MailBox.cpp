#include "MailBox.h"
#ifdef WIN32
#include "win32/Window.h"
#endif

using namespace boost;

CMailBox::CMailBox()
{
    m_callDone = false;
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
    mutex::scoped_lock waitLock(m_waitMutex);
    if(IsPending()) return;
    m_waitCondition.wait(waitLock);
}

void CMailBox::SendCall(const FunctionType& function, bool waitForCompletion)
{
    {
#ifdef WIN32
        if(waitForCompletion)
        {
            assert(!m_callDone);
            m_callDone = false;
        }
#endif		
        mutex::scoped_lock waitLock(m_waitMutex);
        m_calls.push_back(function);
        m_waitCondition.notify_all();
    }
    if(waitForCompletion)
    {
        mutex::scoped_lock doneLock(m_doneNotifyMutex);
#ifdef WIN32
        while(!m_callDone)
        {
            xtime xt;
            xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 100 * 1000000;
            m_callFinished.timed_wait(doneLock, xt);
	        MSG wmmsg;
            while(PeekMessage(&wmmsg, NULL, 0, 0, PM_REMOVE))
		    {
			    TranslateMessage(&wmmsg);
			    DispatchMessage(&wmmsg);
		    }
        }
        m_callDone = false;
#else
        m_callFinished.wait(doneLock);
#endif
    }
}

void CMailBox::ReceiveCall()
{
    FunctionType function;
    {
        mutex::scoped_lock waitLock(m_waitMutex);
        if(!IsPending()) return;
        function = *m_calls.begin();
        m_calls.pop_front();
    }
    function();
    {
        mutex::scoped_lock doneLock(m_doneNotifyMutex);
        m_callFinished.notify_all();
        m_callDone = true;
    }
}
