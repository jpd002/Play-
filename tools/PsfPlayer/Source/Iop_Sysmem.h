#ifndef _IOP_SYSMEM_H_
#define _IOP_SYSMEM_H_

#include "Iop_Module.h"

namespace Iop
{
    class CSysmem : public CModule
    {
    public:
                                CSysmem(uint32, uint32);
        virtual                 ~CSysmem();

        std::string             GetId() const;
        void                    Invoke(CMIPS&, unsigned int);

        uint32                  AllocateMemory(uint32, uint32);
        void                    FreeMemory(uint32);
    private:
        typedef std::map<uint32, uint32> BlockMapType;

        BlockMapType            m_blockMap;
        uint32                  m_memoryBegin;
        uint32                  m_memoryEnd;
        uint32                  m_memorySize;
    };
}


#endif
