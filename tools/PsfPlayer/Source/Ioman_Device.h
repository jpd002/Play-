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
		    enum OPEN_FLAGS
		    {
			    O_RDONLY	= 0x00000001,
			    O_WRONLY	= 0x00000002,
			    O_RDWR		= 0x00000003,
			    O_CREAT		= 0x00000200,
		    };

            virtual                         ~CDevice() {}
            virtual Framework::CStream*     GetFile(uint32, const char*) = 0;
        };
    }
}

#endif
