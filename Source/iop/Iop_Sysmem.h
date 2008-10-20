#ifndef _IOP_SYSMEM_H_
#define _IOP_SYSMEM_H_

#include "Iop_Module.h"
#include "Iop_Stdio.h"
#include "Iop_SifMan.h"

namespace Iop
{
    class CSysmem : public CModule, public CSifModule
    {
    public:
                                CSysmem(uint32, uint32, Iop::CStdio&, CSifMan&);
        virtual                 ~CSysmem();

        std::string             GetId() const;
        void                    Invoke(CMIPS&, unsigned int);
		void			        Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

        uint32                  AllocateMemory(uint32, uint32);
        uint32                  FreeMemory(uint32);

    private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000003
		};

        typedef std::map<uint32, uint32> BlockMapType;

		uint32			        SifAllocate(uint32);
		uint32			        SifAllocateSystemMemory(uint32, uint32, uint32);

        BlockMapType            m_blockMap;
        uint32                  m_memoryBegin;
        uint32                  m_memoryEnd;
        uint32                  m_memorySize;
        Iop::CStdio&            m_stdio;
    };
}


#endif
