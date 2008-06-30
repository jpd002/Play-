#ifndef _VPU_H_
#define _VPU_H_

#include "Types.h"
#include "VIF.h"
#include "MIPS.h"
#include "VuExecutor.h"
#include "Convertible.h"
#include <boost/thread.hpp>
#include <boost/static_assert.hpp>
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CVPU
{
public:
                        CVPU(CVIF&, unsigned int, const CVIF::VPUINIT&);
    virtual             ~CVPU();

    void                JoinThread();

    void                SingleStep();
    virtual void        Reset();
    virtual uint32      GetTOP() const;
    virtual uint32      GetITOP() const;
    virtual void        SaveState(CZipArchiveWriter&);
    virtual void        LoadState(CZipArchiveReader&);
    virtual void        ProcessPacket(CVIF::CFifoStream&);

    CMIPS&              GetContext() const;
    uint8*              GetVuMemory() const;
    bool                IsRunning() const;

protected:
    struct STAT : public convertible<uint32>
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
    };

    struct CYCLE : public convertible<uint32>
    {
        unsigned int    nCL         : 8;
        unsigned int    nWL         : 8;
        unsigned int    reserved    : 16;
    };

#pragma pack(push, 1)
    struct CODE : public convertible<uint32>
    {
        unsigned int    nIMM	: 16;
        unsigned int    nNUM	: 8;
        unsigned int    nCMD	: 7;
        unsigned int    nI		: 1;
    };
    BOOST_STATIC_ASSERT(sizeof(CODE) == sizeof(uint32));
#pragma pack(pop)

    enum ADDMODE
    {
        MODE_NORMAL = 0,
        MODE_OFFSET = 1,
        MODE_DIFFERENCE = 2
    };

    enum MASKOP
    {
        MASK_DATA = 0,
        MASK_ROW = 1,
        MASK_COL = 2,
        MASK_MASK = 3
    };

    void                ExecuteThreadProc();
    void                ExecuteMicro(uint32);
    virtual void        StartMicroProgram(uint32);
    virtual void        ExecuteCommand(CODE, CVIF::CFifoStream&);
    virtual void        Cmd_UNPACK(CODE, CVIF::CFifoStream&, uint32);

    void                Cmd_MPG(CODE, CVIF::CFifoStream&);
    void                Cmd_STROW(CODE, CVIF::CFifoStream&);
    void                Cmd_STMASK(CODE, CVIF::CFifoStream&);

    bool                Unpack_S32(CVIF::CFifoStream&, uint128&);
    bool                Unpack_S16(CVIF::CFifoStream&, uint128&, bool);
    bool                Unpack_V16(CVIF::CFifoStream&, uint128&, unsigned int, bool);
    bool                Unpack_V8(CVIF::CFifoStream&, uint128&, unsigned int, bool);
    bool                Unpack_V32(CVIF::CFifoStream&, uint128&, unsigned int);
    bool                Unpack_V45(CVIF::CFifoStream&, uint128&);

//    void                Unpack_V45(uint32, CVIF::CFifoStream&);
//    void                Unpack_V432(uint32, CVIF::CFifoStream&);
//    void                Unpack_V416(uint32, CVIF::CFifoStream&, bool);

    uint32              GetVbs() const;
    uint32              GetMaskOp(unsigned int, unsigned int) const;

    void                DisassembleCommand(CODE);

    STAT                m_STAT;
    CYCLE               m_CYCLE;
    CODE                m_CODE;
    uint8               m_NUM;
    uint32              m_MODE;
    uint32              m_R[4];
    uint32              m_MASK;
    uint32              m_ITOP;
    uint32              m_ITOPS;
    uint32              m_readTick;
    uint32              m_writeTick;

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
#ifdef _DEBUG
    boost::condition    m_execDoneCondition;
#endif
    boost::mutex        m_execMutex;
    CVuExecutor         m_executor;
};

#endif
