#include "ThreadMsg.h"

CThreadMsg::CThreadMsg()
{
	m_nMessage = false;
#ifdef WIN32
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
}

CThreadMsg::~CThreadMsg()
{
#ifdef WIN32
	CloseHandle(m_hEvent);
#endif
}

unsigned int CThreadMsg::SendMessage(unsigned int nMsg, void* pParam)
{
#ifdef WIN32
	unsigned long nStatus;
	MSG wmmsg;

	m_Msg.nMsg		= nMsg;
	m_Msg.pParam	= pParam;
	m_Msg.nRetValue	= 0;
	m_nMessage		= true;

	ResetEvent(m_hEvent);
	while(1)
	{
		nStatus = MsgWaitForMultipleObjects(1, &m_hEvent, FALSE, INFINITE, QS_SENDMESSAGE);
		if(nStatus == WAIT_OBJECT_0) break;
		//Process the message in queue to prevent a possible deadlock
		if(PeekMessage(&wmmsg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&wmmsg);
			DispatchMessage(&wmmsg);
		}
	}

	return m_Msg.nRetValue;
#endif
	return 0;
}

bool CThreadMsg::GetMessage(MESSAGE* pMsg)
{
#ifdef WIN32
	if(!m_nMessage) return false;
	if(pMsg != NULL)
	{
		pMsg->nMsg		= m_Msg.nMsg;
		pMsg->pParam	= m_Msg.pParam;
	}
#endif
	return true;
}

void CThreadMsg::FlushMessage(unsigned int nRetValue)
{
#ifdef WIN32
	if(!m_nMessage) return;
	m_nMessage = false;
	m_Msg.nRetValue = nRetValue;
	SetEvent(m_hEvent);
#endif
}

bool CThreadMsg::IsMessagePending()
{
	return m_nMessage;
}
