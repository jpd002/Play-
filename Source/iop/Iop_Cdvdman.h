#ifndef _IOP_CDVDMAN_H_
#define _IOP_CDVDMAN_H_

#include "Iop_Module.h"
#include "../ISO9660/ISO9660.h"

namespace Iop
{
    class CCdvdman : public CModule
    {
    public:
                                CCdvdman(uint8*);
        virtual                 ~CCdvdman();

        virtual std::string     GetId() const;
		virtual std::string		GetFunctionName(unsigned int) const;
        virtual void            Invoke(CMIPS&, unsigned int);

        void                    SetIsoImage(CISO9660*);

    private:
        uint32                  CdRead(uint32, uint32, uint32, uint32);
        uint32                  CdGetError();
        uint32                  CdSync(uint32);
        uint32                  CdGetDiskType();
        uint32                  CdDiskReady(uint32);

        CISO9660*               m_image;
        uint8*                  m_ram;
    };
};

#endif
