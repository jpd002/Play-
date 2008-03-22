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
        VU1_TOP     = 0x4400,
        VU1_XGKICK  = 0x4410,
    };

    enum STAT_BITS
    {
        STAT_VBS0   = 0x0001,
        STAT_VBS1   = 0x0100,
    };

                CVIF(CGIF&, uint8*, const VPUINIT&, const VPUINIT&);
    virtual     ~CVIF();

    void        Reset();
    void        SaveState(CZipArchiveWriter&);
    void        LoadState(CZipArchiveReader&);

    uint32      ReceiveDMA1(uint32, uint32, bool);

    uint32*     GetTop1Address();

    void        StopVU(CMIPS*);
    void        ProcessXGKICK(uint32);

    bool        IsVU1Running();
    void        SingleStepVU1();

    uint8*      GetRam() const;
    CGIF&       GetGif() const;
    uint32      GetStat() const;
    void        SetStat(uint32);

private:

    enum STAT1_BITS
    {
        STAT1_DBF   = 0x80,
    };

    uint32      m_VPU_STAT;
    CVPU*       m_pVPU[2];
    CGIF&       m_gif;
    uint8*      m_ram;
};

#endif
