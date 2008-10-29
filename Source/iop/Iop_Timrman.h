#ifndef _TIMRMAN_H_
#define _TIMRMAN_H_

#include "Iop_Module.h"

namespace Iop
{
    class CTimrman : public CModule
    {
    public:
                        CTimrman();
        virtual         ~CTimrman();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        int             AllocHardTimer(int, int, int);
    };
}

#endif
