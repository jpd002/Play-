#ifndef _IOP_SYSCLIB_H_
#define _IOP_SYSCLIB_H_

#include "IOP_Module.h"

namespace Iop
{
    class CSysclib : public CModule
    {
    public:
                        CSysclib(uint8*);
        virtual         ~CSysclib();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        void            __memset(void*, int, unsigned int);
        uint32          __strlen(const char*);
        void            __strncpy(char*, const char*, unsigned int);
        uint32          __strtol(const char*, unsigned int);
        uint8*          m_ram;
    };
}

#endif
