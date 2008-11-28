#ifndef _IOP_SYSCLIB_H_
#define _IOP_SYSCLIB_H_

#include "IOP_Module.h"
#include "IOP_Stdio.h"

namespace Iop
{
    class CSysclib : public CModule
    {
    public:
                        CSysclib(uint8*, CStdio&);
        virtual         ~CSysclib();

        std::string     GetId() const;
		std::string		GetFunctionName(unsigned int) const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        void            __memcpy(void*, const void*, unsigned int);
        void            __memset(void*, int, unsigned int);
        uint32          __sprintf(CMIPS& context);
        uint32          __strlen(const char*);
        void            __strcpy(char*, const char*);
        void            __strncpy(char*, const char*, unsigned int);
        uint32          __strtol(const char*, unsigned int);
        uint8*          m_ram;
        CStdio&         m_stdio;
    };
}

#endif
