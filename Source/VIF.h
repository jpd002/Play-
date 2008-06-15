#ifndef _VIF_H_
#define _VIF_H_

#include "Types.h"
#include "MIPS.h"
#include "GIF.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CVPU;

class CVIF
{
public:
    struct VPUINIT
    {
        VPUINIT(uint8* microMem, uint8* vuMem, CMIPS* context) :
        microMem(microMem),
        vuMem(vuMem),
        context(context)
        {

        }

        uint8* microMem;
        uint8* vuMem;
        CMIPS* context;
    };

    enum VU1REGISTERS
    {
        VU_TOP      = 0x8400,
        VU_XGKICK   = 0x8410,
        VU_ITOP     = 0x8420,
    };

    enum STAT_BITS
    {
        STAT_VBS0   = 0x0001,
        STAT_VBS1   = 0x0100,
    };

                CVIF(CGIF&, uint8*, const VPUINIT&, const VPUINIT&);
    virtual     ~CVIF();

    void        JoinThreads();

    void        Reset();
    void        SaveState(CZipArchiveWriter&);
    void        LoadState(CZipArchiveReader&);

    uint32      ReceiveDMA0(uint32, uint32, bool);
    uint32      ReceiveDMA1(uint32, uint32, bool);

    uint32      GetITop0() const;
    uint32      GetITop1() const;
    uint32      GetTop1() const;

    void        StopVU(CMIPS*);
    void        ProcessXGKICK(uint32);

    bool        IsVu0DebuggingEnabled() const;
    bool        IsVu1DebuggingEnabled() const;

    bool        IsVU0Running() const;
    bool        IsVU1Running() const;

    void        SingleStepVU0();
    void        SingleStepVU1();

    uint8*      GetRam() const;
    CGIF&       GetGif() const;
    uint32      GetStat() const;
    void        SetStat(uint32);

    class CFifoStream
    {
    public:
                        CFifoStream(uint8*);
        virtual         ~CFifoStream();

        uint32          GetAddress() const;
        uint32          GetSize() const;
        void            Read(void*, uint32);
        void            Flush();
        void            Align32();
        void            SetDmaParams(uint32, uint32);

    private:
        void            SyncBuffer();

        enum
        {
            BUFFERSIZE = 0x10
        };

        uint128         m_buffer;
        uint32          m_position;
        uint8*          m_ram;
        uint32          m_address;
        uint32          m_nextAddress;
        uint32          m_endAddress;
    };

private:
    enum STAT1_BITS
    {
        STAT1_DBF   = 0x80,
    };

    uint32          m_VPU_STAT;
    CVPU*           m_pVPU[2];
    CGIF&           m_gif;
    uint8*          m_ram;
    CFifoStream*    m_stream[2];
};

#endif
