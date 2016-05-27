#ifndef _ARGUMENT_ITERATOR_H_
#define _ARGUMENT_ITERATOR_H_

#include "../MIPS.h"

class CArgumentIterator
{
public:
                    CArgumentIterator(CMIPS&);
					CArgumentIterator(CMIPS&, const char*);
    virtual         ~CArgumentIterator();
    uint32          GetNext();

private:
    unsigned int    m_current;
    CMIPS&          m_context;
	const char*		m_args;
};

#endif
