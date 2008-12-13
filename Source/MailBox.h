#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include <functional>
#include <deque>
#include <boost/thread.hpp>

class CMailBox
{
public:
                        CMailBox();
    virtual             ~CMailBox();

    typedef std::tr1::function<void ()> FunctionType;

    void                SendCall(const FunctionType&, bool = false);

    bool                IsPending() const;
    void                ReceiveCall();
    void                WaitForCall();

private:
	struct MESSAGE
	{
		FunctionType	function;
		bool			sync;
	};

    typedef std::deque<MESSAGE> FunctionCallQueue;

    FunctionCallQueue   m_calls;
    boost::mutex        m_waitMutex;
    boost::mutex        m_doneNotifyMutex;
    boost::condition    m_callFinished;
    boost::condition    m_waitCondition;
};

#endif
