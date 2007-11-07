#ifndef _IOMAN_DEVICE_H_
#define _IOMAN_DEVICE_H_

#include "Stream.h"

namespace Iop
{
    namespace Ioman
    {
        class CDevice
        {
        public:
            virtual                         ~CDevice() {}
            virtual Framework::CStream*     GetFile(uint32, const char*) = 0;
        };
    }
}

#endif
