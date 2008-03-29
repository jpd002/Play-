#ifndef _VPU_H_
#define _VPU_H_

#include "Types.h"
#include "VIF.h"
#include "MIPS.h"
#include "VuExecutor.h"
#include <boost/thread.hpp>
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CVPU
{
public:
                        CVPU(CVIF&, unsigned int, const CVIF::VPUINIT&);
    virtual             ~CVPU();
    void                SingleStep();
    virtual void        Reset();
    virtual uint32      GetTOP();
    virtual void        SaveState(CZipArchiveWriter&);
    virtual void        LoadState(CZipArchiveReader&);
    virtual void        ProcessPacket(CVIF::CFifoStream&);

    CMIPS&              GetContext() const;
    uint8*              GetVuMemory() const;

protected:
    struct STAT
    {
        unsigned int    nVPS        : 2;
        unsigned int    nVEW        : 1;
        unsigned int    nReserved0  : 3;
        unsigned int    nMRK        : 1;
        unsigned int    nDBF        : 1;
        unsigned int    nVSS        : 1;
        unsigned int    nVFS        : 1;
        unsigned int    nVIS        : 1;
        unsigned int    nINT        : 1;
        unsigned int    nER0        : 1;
        unsigned int    nER1        : 1;
        unsigned int    nReserved2  : 10;
        unsigned int    nFQC        : 4;
        unsigned int    nReserved3  : 4;

        STAT& operator =(uint32 value)
        {
            (*this) = *reinterpret_cast<STAT*>(&value);
            return (*this);
        }

        operator uint32()
        {
            return *reinterpret_cast<uint32*>(this);
        }
    };

    struct CYCLE
    {
        unsigned int    nCL         : 8;
        unsigned int    nWL         : 8;
        unsigned int    reserved    : 16;

        CYCLE& operator =(uint32 value)
        {
            (*this) = *reinterpret_cast<CYCLE*>(&value);
            return (*this);
        }

        operator uint32()
        {
            return *reinterpret_cast<uint32*>(this);
        }
    };

#pragma pack(push, 1)
    struct CODE
    {
        unsigned int    nIMM	: 16;
        unsigned int    nNUM	: 8;
        unsigned int    nCMD	: 7;
        unsigned int    nI		: 1;

        CODE& operator =(uint32 value)
        {
            (*this) = *reinterpret_cast<CODE*>(&value);
            return (*this);
        }

        operator uint32()
        {
            return *reinterpret_cast<uint32*>(this);
        }
    };
#pragma pack(pop)

    void                ExecuteThreadProc();
    void                ExecuteMicro(uint32, uint32);
    virtual uint32      ExecuteCommand(CODE, CVIF::CFifoStream&);
    virtual uint32      Cmd_UNPACK(CODE, CVIF::CFifoStream&);

    uint32              Cmd_MPG(CODE, CVIF::CFifoStream&);

    void                Unpack_V45(uint32, CVIF::CFifoStream&);
    void                Unpack_V432(uint32, CVIF::CFifoStream&);

    uint32              GetVbs() const;

    STAT                m_STAT;
    CYCLE               m_CYCLE;
    CODE                m_CODE;
    uint8               m_NUM;

    uint128             m_buffer;

    uint8*              m_pMicroMem;
    uint8*              m_pVUMem;
    CMIPS*              m_pCtx;
    CVIF&               m_vif;

    unsigned int        m_vpuNumber;

    bool                m_endThread;
    bool                m_paused;
    boost::thread*      m_execThread;
    boost::condition    m_execCondition;
    boost::mutex        m_execMutex;
    CVuExecutor         m_executor;
};

#endif
