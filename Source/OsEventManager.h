#ifndef _OSEVENTMANAGER_H_
#define _OSEVENTMANAGER_H_

#include "Singleton.h"
#include "xml/Node.h"
#include <vector>

class COsEventManager : public CSingleton<COsEventManager>
{
public:
	friend					CSingleton;

	struct OSEVENT
	{
		unsigned int		nTime;
		unsigned int		nThreadId;
		unsigned int		nEventType;
	};

	void					Begin(const char*);
	void					InsertEvent(OSEVENT);
	void					Flush();
	Framework::Xml::CNode*	GetEvents();

private:
	enum
	{
		RESERVE = 500,
	};

	typedef std::vector<OSEVENT> EventListType;

							COsEventManager();
	virtual					~COsEventManager();

	unsigned int			m_nCurrentTime;
	EventListType			m_Events;
	std::string				m_sExecutableName;
};

#endif
