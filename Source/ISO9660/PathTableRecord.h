#ifndef _ISO9660_PATHTABLERECORD_H_
#define _ISO9660_PATHTABLERECORD_H_

#include <string>
#include "Types.h"
#include "Stream.h"

namespace ISO9660
{
    class CPathTableRecord
    {
    public:
                        CPathTableRecord(Framework::CStream*);
                        ~CPathTableRecord();

        uint8           GetNameLength() const;
        uint32          GetAddress() const;
        uint32          GetParentRecord() const;
        const char*     GetName() const;

    private:
        uint8           m_nNameLength;
        uint8           m_nExLength;
        uint32          m_nLocation;
        uint16          m_nParentDir;
        std::string     m_sDirectory;
    };
}

#endif
