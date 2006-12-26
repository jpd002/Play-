#ifndef _OSEVENTMANAGER_H_
#define _OSEVENTMANAGER_H_

#include "Singleton.h"
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

private:
	enum
	{
		RESERVE = 100,
	};

	typedef std::vector<OSEVENT> EventListType;

							COsEventManager();
	virtual					~COsEventManager();

	void					Flush();

	unsigned int			m_nCurrentTime;
	EventListType			m_Events;
	std::string				m_sExecutableName;
};

#endif
