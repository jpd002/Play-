#ifndef _IOP_LOADCORE_H_
#define _IOP_LOADCORE_H_

#include "Iop_Module.h"
#include "Iop_SifMan.h"

class CIopBios;

namespace Iop
{
    class CLoadcore : public CModule, public CSifModule
    {
    public:
                        CLoadcore(CIopBios&, uint8*, CSifMan&);
        virtual         ~CLoadcore();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);
        void            Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

    private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000006
		};

        uint32          RegisterLibraryEntries(uint32*);
        void            LoadModule(uint32*, uint32, uint32*, uint32);
        void            LoadModuleFromMemory(uint32*, uint32, uint32*, uint32);
        void            Initialize(uint32*, uint32, uint32*, uint32);

        CIopBios&       m_bios;
        uint8*          m_ram;
    };
}

#endif
