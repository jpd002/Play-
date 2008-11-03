#ifndef _BIOS_H_
#define _BIOS_H_

class CBios
{
public:
    virtual         ~CBios() {}
    virtual void    HandleException() = 0;
    virtual void    HandleInterrupt() = 0;

private:

};

#endif
