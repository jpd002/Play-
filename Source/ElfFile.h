#ifndef _ELFFILE_H_
#define _ELFFILE_H_

#include "ELF.h"

class CElfFileContainer
{
public:
                CElfFileContainer(Framework::CStream&);
    virtual     ~CElfFileContainer();

    uint8*      GetFileContent() const;

private:
    uint8*      m_content;

};

class CElfFile : protected CElfFileContainer, public CELF
{
public:
                CElfFile(Framework::CStream&);
    virtual     ~CElfFile();

private:

};

#endif
