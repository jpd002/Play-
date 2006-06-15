#ifndef _THREADMSG_H_
#define _THREADMSG_H_

#include <windows.h>

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
	HANDLE				m_hEvent;
	MESSAGE				m_Msg;		
	bool				m_nMessage;
};

#endif