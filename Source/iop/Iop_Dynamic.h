#ifndef _IOP_DYNAMIC_H_
#define _IOP_DYNAMIC_H_

#include "Iop_Module.h"

namespace Iop
{
    class CDynamic : public CModule
    {
    public:
                        CDynamic(uint32*);
        virtual         ~CDynamic();

        std::string     GetId() const;
		std::string		GetFunctionName(unsigned int) const;
        void            Invoke(CMIPS&, unsigned int);

        uint32*         GetExportTable() const;

    private:
        uint32*         m_exportTable;
        std::string     m_name;
    };
}

#endif
