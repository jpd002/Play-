#ifndef _IOMAN_PSF_H_
#define _IOMAN_PSF_H_

#include "Ioman_Device.h"
#include "PsfFs.h"

namespace Iop
{
    namespace Ioman
    {
        class CPsf : public CDevice
        {
        public:
                                    CPsf(CPsfFs&);
            virtual                 ~CPsf();

            Framework::CStream*     GetFile(uint32, const char*);

        private:
            CPsfFs&                 m_fileSystem;
        };
    }
}

#endif
