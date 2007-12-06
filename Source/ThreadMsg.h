#ifndef _THREADMSG_H_
#define _THREADMSG_H_

#ifdef WIN32
#include <windows.h>
#endif

class CThreadMsg
{
public:
	struct MESSAGE
	{
		unsigned int	nMsg;
		unsigned int	nRetValue;
		void*			pParam;
	};
						CThreadMsg();
						~CThreadMsg();
	unsigned int		SendMessage(unsigned int, void*);
	bool				GetMessage(MESSAGE*);
	void				FlushMessage(unsigned int);
	bool				IsMessagePending();

private:
#ifdef WIN32
	HANDLE				m_hEvent;
#endif
	MESSAGE				m_Msg;		
	bool				m_nMessage;
};

#endif