#ifndef _IOP_DMAC_H_
#define _IOP_DMAC_H_

namespace Iop
{
    class CDmac
    {
    public:
                    CDmac();
        virtual     ~CDmac();

        enum DMAC_ZONE1
        {
            DMAC_ZONE1_START    = 0x1F801080,
            DMAC_ZONE1_END      = 0x1F8010FF,
        };

    private:

    };
}

#endif
