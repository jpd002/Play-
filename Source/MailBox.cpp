#include "MailBox.h"

using namespace boost;

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
    mutex::scoped_lock waitLock(m_waitMutex);
    if(IsPending()) return;
    m_waitCondition.wait(waitLock);
}

void CMailBox::SendCall(const FunctionType& function)
{
    mutex::scoped_lock waitLock(m_waitMutex);
    m_calls.push_back(function);
    m_waitCondition.notify_all();
//    m_callFinished.wait(waitLock);
}

void CMailBox::ReceiveCall()
{
    mutex::scoped_lock waitLock(m_waitMutex);
    if(!IsPending()) return;
    FunctionType function = *m_calls.begin();
    m_calls.pop_front();
    function();
    m_callFinished.notify_one();
}
