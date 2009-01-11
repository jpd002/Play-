#ifndef _ISODEVICE_H_
#define _ISODEVICE_H_

#include "Ioman_Device.h"
#include "../ISO9660/ISO9660.h"

namespace Iop
{
    namespace Ioman
    {
        class CIsoDevice : public CDevice
        {
        public:
                                            CIsoDevice(CISO9660*&);
            virtual                         ~CIsoDevice();
            virtual Framework::CStream*     GetFile(uint32, const char*);

        private:
            static char                     FixSlashes(char);

            CISO9660*&                      m_iso;
        };
    }
}

#endif
