#ifndef _BIOS_H_
#define _BIOS_H_

#include "Types.h"

class CBios
{
public:
    virtual         ~CBios() {}
    virtual void    HandleException() = 0;
    virtual void    HandleInterrupt() = 0;
	virtual void	CountTicks(uint32) = 0;

private:

};

#endif
