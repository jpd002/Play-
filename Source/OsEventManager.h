#ifndef _OSEVENTMANAGER_H_
#define _OSEVENTMANAGER_H_

#include "Singleton.h"
#include "xml/Node.h"
#include "Types.h"
#include <vector>

class COsEventManager : public CSingleton<COsEventManager>
{
public:
	friend					CSingleton;

	class COsEvent
	{
	public:
		unsigned int		nTime;
		unsigned int		nThreadId;
		unsigned int		nEventType;
		uint32				nAddress;
		std::string			sDescription;
	};

	void					Begin(const char*);
	void					InsertEvent(COsEvent);
	void					Flush();
	Framework::Xml::CNode*	GetEvents();

private:
	enum
	{
		RESERVE = 500,
	};

	typedef std::vector<COsEvent> EventListType;

							COsEventManager();
	virtual					~COsEventManager();

	unsigned int			m_nCurrentTime;
	EventListType			m_Events;
	std::string				m_sRecordPath;
};

#endif
