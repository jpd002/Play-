#ifndef _MEMORYFUNCTION_H_
#define _MEMORYFUNCTION_H_

#include <boost/utility.hpp>

class CMemoryFunction : public boost::noncopyable
{
public:
				CMemoryFunction(const void*, size_t);
	virtual		~CMemoryFunction();

	void		operator()(void*);

private:
	void*		m_code;
};

#endif
