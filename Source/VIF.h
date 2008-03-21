#ifndef _VIF_H_
#define _VIF_H_

#include <boost/thread.hpp>
#include "Types.h"
#include "Stream.h"
#include "MIPS.h"
#include "GIF.h"

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

                CVIF(CGIF&, uint8*, const VPUINIT&, const VPUINIT&);
    virtual     ~CVIF();

    void        Reset();
    void        SaveState(Framework::CStream*);
    void        LoadState(Framework::CStream*);

    uint32      ReceiveDMA1(uint32, uint32, bool);

    uint32*     GetTop1Address();

    void        StopVU(CMIPS*);
    void        ProcessXGKICK(uint32);

    bool        IsVU1Running();
    bool        IsPauseNeeded() const;
    void        SetPauseNeeded(bool);

private:
    enum STAT_BITS
    {
        STAT_VBS0   = 0x0001,
        STAT_VBS1   = 0x0100,
    };

    enum STAT1_BITS
    {
        STAT1_DBF   = 0x80,
    };

#pragma pack(push, 1)
    struct CODE
    {
        unsigned int    nIMM	: 16;
        unsigned int    nNUM	: 8;
        unsigned int    nCMD	: 7;
        unsigned int    nI		: 1;
    };
#pragma pack(pop)

    class CVPU
    {
    public:
                            CVPU(CVIF&, const VPUINIT&);
        virtual             ~CVPU();
        virtual void        Reset();
        virtual uint32*     GetTOP();
        virtual void        SaveState(Framework::CStream*);
        virtual void        LoadState(Framework::CStream*);
        virtual void        ProcessPacket(uint32, uint32);

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
        };

        struct CYCLE
        {
            unsigned int    nCL         : 8;
            unsigned int    nWL         : 8;

            CYCLE& operator =(unsigned int nValue)
            {
                (*this) = *(CYCLE*)&nValue;
                return (*this);
            }
        };

        void                ExecuteThreadProc();
        void                ExecuteMicro(uint32, uint32);
        virtual uint32      ExecuteCommand(CODE, uint32, uint32);
        virtual uint32      Cmd_UNPACK(CODE, uint32, uint32);

        uint32              Cmd_MPG(CODE, uint32, uint32);

        uint32              Unpack_V45(uint32, uint32, uint32);
        uint32              Unpack_V432(uint32, uint32, uint32);

        STAT                m_STAT;
        CYCLE               m_CYCLE;
        CODE                m_CODE;
        uint8               m_NUM;

        uint8*              m_pMicroMem;
        uint8*              m_pVUMem;
        CMIPS*              m_pCtx;
        CVIF&               m_vif;

        bool                m_endThread;
        boost::thread*      m_execThread;
        boost::condition    m_execCondition;
	};

    class CVPU1 : public CVPU
    {
    public:
                        CVPU1(CVIF&, const VPUINIT&);
        virtual void    SaveState(Framework::CStream*);
        virtual void    LoadState(Framework::CStream*);
        virtual uint32* GetTOP();
        virtual void    Reset();

    protected:
        virtual uint32  ExecuteCommand(CODE, uint32, uint32);
        virtual uint32  Cmd_DIRECT(CODE, uint32, uint32);
        virtual uint32  Cmd_UNPACK(CODE, uint32, uint32);

    private:
        uint32          m_BASE;
        uint32          m_OFST;
        uint32          m_TOP;
        uint32          m_TOPS;
    };

    uint8*      GetRam() const;
    CGIF&       GetGif() const;
    uint32      GetStat() const;
    void        SetStat(uint32);

    uint32      m_VPU_STAT;
    CVPU*       m_pVPU[2];
    CGIF&       m_gif;
    uint8*      m_ram;
    bool        m_pauseNeeded;
};

#endif
