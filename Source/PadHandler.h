#ifndef _PADHANDLER_H_
#define _PADHANDLER_H_

#include "PadInterface.h"
#include <list>
#include <functional>

class CPadHandler
{
public:
	typedef std::function<CPadHandler*(void)> FactoryFunction;

	CPadHandler() = default;
	virtual ~CPadHandler() = default;
	virtual void Update(uint8*) = 0;
	void InsertListener(CPadInterface*);
	void RemoveAllListeners();

protected:
	typedef std::list<CPadInterface*> ListenerList;
	ListenerList m_interfaces;
};

#endif
