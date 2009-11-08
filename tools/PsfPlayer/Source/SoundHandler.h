#ifndef _SOUNDHANDLER_H_
#define _SOUNDHANDLER_H_

#include "Types.h"

class CSoundHandler
{
public:
    virtual				~CSoundHandler() {}

	virtual void		Reset() = 0;
    virtual void        Write(int16*, unsigned int, unsigned int) = 0;
	virtual bool		HasFreeBuffers() = 0;
    virtual void        RecycleBuffers() = 0;

private:

};

#endif
