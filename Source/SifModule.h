#ifndef _SIFMODULE_H_
#define _SIFMODULE_H_

class CSifModule
{
public:
    virtual         ~CSifModule() {}
    virtual void    Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) = 0;
};

#endif
