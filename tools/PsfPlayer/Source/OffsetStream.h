#ifndef _OFFSETSTREAM_H_
#define _OFFSETSTREAM_H_

#include "Stream.h"

namespace Framework
{
    class COffsetStream : public CStream
    {
    public:
                    COffsetStream(CStream&, uint64);
        virtual     ~COffsetStream();

        void        Seek(int64, Framework::STREAM_SEEK_DIRECTION);
        uint64      Tell();
        uint64      Read(void*, uint64);
        uint64      Write(const void*, uint64);
        bool        IsEOF();

    private:
        CStream&    m_stream;
        uint64      m_offset;
    };
}

#endif
