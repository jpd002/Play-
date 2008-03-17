#ifndef _ISO9660_DIRECTORYRECORD_H_
#define _ISO9660_DIRECTORYRECORD_H_

#include "Types.h"
#include "Stream.h"

namespace ISO9660
{
    class CDirectoryRecord
    {
    public:
                        CDirectoryRecord();
                        CDirectoryRecord(Framework::CStream*);
                        ~CDirectoryRecord();

        bool            IsDirectory() const;
        uint8           GetLength() const;
        const char*     GetName() const;
        uint32          GetPosition() const;
        uint32          GetDataLength() const;

    private:
        uint8           m_nLength;
        uint8           m_nExLength;
        uint32          m_nPosition;
        uint32          m_nDataLength;
        uint8           m_nFlags;
        char            m_sName[256];
    };
}

#endif
