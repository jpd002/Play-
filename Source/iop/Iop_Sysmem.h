#ifndef _IOP_SYSMEM_H_
#define _IOP_SYSMEM_H_

#include "Iop_Module.h"
#include "Iop_Stdio.h"
#include "Iop_SifMan.h"
#include "../OsStructManager.h"

namespace Iop
{
    class CSysmem : public CModule, public CSifModule
    {
    public:
		struct BLOCK
		{
			uint32	isValid;
			uint32	nextBlock;
			uint32	address;
			uint32	size;
		};

		enum
		{
			MAX_BLOCKS = 256,
		};

                                CSysmem(uint8*, uint32, uint32, uint32, Iop::CStdio&, CSifMan&);
        virtual                 ~CSysmem();

        std::string             GetId() const;
		std::string				GetFunctionName(unsigned int) const;
        void                    Invoke(CMIPS&, unsigned int);
		bool			        Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

        uint32                  AllocateMemory(uint32, uint32);
        uint32                  FreeMemory(uint32);

    private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000003
		};

		typedef COsStructManager<BLOCK> BlockListType;

		uint32			        SifAllocate(uint32);
		uint32			        SifAllocateSystemMemory(uint32, uint32, uint32);
        uint32                  SifFreeMemory(uint32);

		BlockListType			m_blocks;
        uint32					m_memoryBegin;
        uint32					m_memoryEnd;
        uint32					m_memorySize;
		uint32					m_headBlockId;
        Iop::CStdio&			m_stdio;
    };
}


#endif
