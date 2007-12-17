#ifndef _MEMORYSTATEFILE_H_
#define _MEMORYSTATEFILE_H_

#include "zip/ZipFile.h"

class CMemoryStateFile : public CZipFile
{
public:
                    CMemoryStateFile(const char*, void*, size_t);
    virtual         ~CMemoryStateFile();

    virtual void    Write(Framework::CStream&);

private:
    void*           m_memory;
    size_t          m_size;
};

#endif
