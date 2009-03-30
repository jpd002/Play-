#ifndef _X86ASSEMBLER_H_
#define _X86ASSEMBLER_H_

#include "Types.h"
#include <functional>
#include <map>

class CX86Assembler
{
public:
    enum REGISTER
    {
        rAX = 0,
        rCX,
        rDX,
        rBX,
        rSP,
        rBP,
        rSI,
        rDI,
        r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15,
    };

    enum XMMREGISTER
    {
        xMM0 = 0,
        xMM1,
        xMM2,
        xMM3,
        xMM4,
        xMM5,
        xMM6,
        xMM7,
        xMM8,
        xMM9,
        xMM10,
        xMM11,
        xMM12,
        xMM13,
        xMM14,
        xMM15,
    };

    typedef std::tr1::function<void (uint8)>                WriteFunctionType;
    typedef std::tr1::function<void (unsigned int, uint8)>  WriteAtFunctionType;
    typedef std::tr1::function<size_t ()>                   TellFunctionType;
    typedef unsigned int LABEL;

    class CAddress
    {
    public:
                        CAddress();

        bool            nIsExtendedModRM;
        bool            nIsExtendedSib;

        union MODRMBYTE
        {
            struct
            {
                unsigned int nRM : 3;
                unsigned int nFnReg : 3;
                unsigned int nMod : 2;
            };
            uint8 nByte;
        };

        union SIB
        {
            struct
            {
                unsigned int base : 3;
                unsigned int index : 3;
                unsigned int scale : 2;
            };
            uint8 byteValue;
        };

        MODRMBYTE       ModRm;
        SIB             sib;
        uint32          nOffset;

        bool            HasSib() const;
        void            Write(const WriteFunctionType&);
    };

                                            CX86Assembler(
                                                const WriteFunctionType&, 
                                                const WriteAtFunctionType&,
                                                const TellFunctionType&);
    virtual                                 ~CX86Assembler();

    static CAddress                         MakeRegisterAddress(REGISTER);
    static CAddress                         MakeXmmRegisterAddress(XMMREGISTER);
    static CAddress                         MakeByteRegisterAddress(REGISTER);
    static CAddress                         MakeIndRegAddress(REGISTER);
    static CAddress                         MakeIndRegOffAddress(REGISTER, uint32);
    static CAddress                         MakeBaseIndexScaleAddress(REGISTER, REGISTER, uint8);

    LABEL                                   CreateLabel();
    void                                    MarkLabel(LABEL);
    void                                    ResolveLabelReferences();

    void                                    AdcEd(REGISTER, const CAddress&);
	void									AdcId(const CAddress&, uint32);
    void                                    AddEd(REGISTER, const CAddress&);
    void                                    AddId(const CAddress&, uint32);
    void                                    AndEd(REGISTER, const CAddress&);
    void                                    AndIb(const CAddress&, uint8);
    void                                    AndId(const CAddress&, uint32);
    void                                    BsrEd(REGISTER, const CAddress&);
    void                                    CallEd(const CAddress&);
    void                                    CmovsEd(REGISTER, const CAddress&);
    void                                    CmovnsEd(REGISTER, const CAddress&);
    void                                    CmpEd(REGISTER, const CAddress&);
    void                                    CmpEq(REGISTER, const CAddress&);
    void                                    CmpIb(const CAddress&, uint8);
    void                                    CmpId(const CAddress&, uint32);
    void                                    CmpIq(const CAddress&, uint64);
    void                                    Cdq();
    void                                    DivEd(const CAddress&);
    void                                    IdivEd(const CAddress&);
    void                                    ImulEd(const CAddress&);
    void                                    JaeJb(LABEL);
    void                                    JcJb(LABEL);
    void                                    JeJb(LABEL);
    void                                    JmpJb(LABEL);
    void                                    JneJb(LABEL);
    void                                    JnoJb(LABEL);
    void                                    JnsJb(LABEL);
    void                                    LeaGd(REGISTER, const CAddress&);
    void                                    MovEd(REGISTER, const CAddress&);
    void                                    MovEq(REGISTER, const CAddress&);
    void                                    MovGd(const CAddress&, REGISTER);
    void                                    MovId(REGISTER, uint32);
    void                                    MovsxEb(REGISTER, const CAddress&);
    void                                    MovsxEw(REGISTER, const CAddress&);
    void                                    MovzxEb(REGISTER, const CAddress&);
    void                                    MulEd(const CAddress&);
    void                                    NegEd(const CAddress&);
    void                                    Nop();
    void                                    NotEd(const CAddress&);
    void                                    OrEd(REGISTER, const CAddress&);
    void                                    OrId(const CAddress&, uint32);
    void                                    Pop(REGISTER);
    void                                    Push(REGISTER);
    void                                    PushEd(const CAddress&);
    void                                    PushId(uint32);
    void                                    RepMovsb();
    void                                    Ret();
    void                                    SarEd(const CAddress&);
    void                                    SarEd(const CAddress&, uint8);
    void                                    SbbEd(REGISTER, const CAddress&);
    void                                    SbbId(const CAddress&, uint32);
    void                                    SetbEb(const CAddress&);
    void                                    SetbeEb(const CAddress&);
    void                                    SeteEb(const CAddress&);
    void                                    SetneEb(const CAddress&);
    void                                    SetlEb(const CAddress&);
	void									SetleEb(const CAddress&);
    void                                    SetgEb(const CAddress&);
    void                                    ShrEd(const CAddress&);
    void                                    ShrEd(const CAddress&, uint8);
    void                                    ShrdEd(const CAddress&, REGISTER);
    void                                    ShrdEd(const CAddress&, REGISTER, uint8);
    void                                    ShlEd(const CAddress&);
    void                                    ShlEd(const CAddress&, uint8);
    void                                    ShldEd(const CAddress&, REGISTER);
    void                                    ShldEd(const CAddress&, REGISTER, uint8);
    void                                    SubEd(REGISTER, const CAddress&);
    void                                    SubId(const CAddress&, uint32);
    void                                    TestEb(REGISTER, const CAddress&);
    void                                    TestEd(REGISTER, const CAddress&);
    void                                    XorEd(REGISTER, const CAddress&);
	void									XorId(const CAddress&, uint32);
    void                                    XorGd(const CAddress&, REGISTER);
    void                                    XorGq(const CAddress&, REGISTER);

    //FPU
    void                                    FldEd(const CAddress&);
    void                                    FildEd(const CAddress&);
    void                                    FstpEd(const CAddress&);
    void                                    FistpEd(const CAddress&);
    void                                    FisttpEd(const CAddress&);
    void                                    FaddpSt(uint8);
    void                                    FsubpSt(uint8);
    void                                    FmulpSt(uint8);
    void                                    FdivpSt(uint8);
    void                                    Fwait();
    void                                    Fsin();
    void                                    FnstcwEw(const CAddress&);
    void                                    FldcwEw(const CAddress&);

    //SSE

    enum SSE_CMP_TYPE
    {
        SSE_CMP_EQ = 0,
        SSE_CMP_LT = 1,
        SSE_CMP_LE = 2,
        SSE_CMP_UNORD = 3,
        SSE_CMP_NEQ = 4,
        SSE_CMP_NLT = 5,
        SSE_CMP_NLE = 6,
        SSE_CMP_ORD = 7,
    };

    void                                    MovdVo(XMMREGISTER, const CAddress&);
    void                                    MovdquVo(const CAddress&, XMMREGISTER);
    void                                    MovdquVo(XMMREGISTER, const CAddress&);
    void                                    MovapsVo(const CAddress&, XMMREGISTER);
    void                                    MovapsVo(XMMREGISTER, const CAddress&);
    void                                    PackssdwVo(XMMREGISTER, const CAddress&);
    void                                    PackuswbVo(XMMREGISTER, const CAddress&);
    void                                    PaddwVo(XMMREGISTER, const CAddress&);
    void                                    PandVo(XMMREGISTER, const CAddress&);
    void                                    PandnVo(XMMREGISTER, const CAddress&);
    void                                    PcmpeqdVo(XMMREGISTER, const CAddress&);
    void                                    PcmpgtwVo(XMMREGISTER, const CAddress&);
    void                                    PmaxswVo(XMMREGISTER, const CAddress&);
    void                                    PminswVo(XMMREGISTER, const CAddress&);
    void                                    PorVo(XMMREGISTER, const CAddress&);
    void                                    PsllwVo(XMMREGISTER, uint8);
    void                                    PslldVo(XMMREGISTER, uint8);
    void                                    PsrawVo(XMMREGISTER, uint8);
    void                                    PsradVo(XMMREGISTER, uint8);
    void                                    PsrlwVo(XMMREGISTER, uint8);
    void                                    PsrldVo(XMMREGISTER, uint8);
    void                                    PsubbVo(XMMREGISTER, const CAddress&);
    void                                    PsubdVo(XMMREGISTER, const CAddress&);
    void                                    PunpcklbwVo(XMMREGISTER, const CAddress&);
    void                                    PunpcklwdVo(XMMREGISTER, const CAddress&);
    void                                    PunpckldqVo(XMMREGISTER, const CAddress&);
    void                                    PunpckhbwVo(XMMREGISTER, const CAddress&);
    void                                    PunpckhdqVo(XMMREGISTER, const CAddress&);
    void                                    PxorVo(XMMREGISTER, const CAddress&);

    void                                    MovssEd(const CAddress&, XMMREGISTER);
    void                                    MovssEd(XMMREGISTER, const CAddress&);
    void                                    AddssEd(XMMREGISTER, const CAddress&);
    void                                    SubssEd(XMMREGISTER, const CAddress&);
    void                                    MulssEd(XMMREGISTER, const CAddress&);
    void                                    DivssEd(XMMREGISTER, const CAddress&);
    void                                    RcpssEd(XMMREGISTER, const CAddress&);
    void                                    RsqrtssEd(XMMREGISTER, const CAddress&);
    void                                    SqrtssEd(XMMREGISTER, const CAddress&);
    void                                    CmpssEd(XMMREGISTER, const CAddress&, SSE_CMP_TYPE);
    void                                    Cvtsi2ssEd(XMMREGISTER, const CAddress&);
    void                                    Cvttss2siEd(REGISTER, const CAddress&);
    void                                    Cvtdq2psVo(XMMREGISTER, const CAddress&);
    void                                    Cvttps2dqVo(XMMREGISTER, const CAddress&);

    void                                    AddpsVo(XMMREGISTER, const CAddress&);
    void                                    DivpsVo(XMMREGISTER, const CAddress&);
    void                                    MaxpsVo(XMMREGISTER, const CAddress&);
    void                                    MinpsVo(XMMREGISTER, const CAddress&);
    void                                    MulpsVo(XMMREGISTER, const CAddress&);
    void                                    SubpsVo(XMMREGISTER, const CAddress&);
    void                                    ShufpsVo(XMMREGISTER, const CAddress&, uint8);

private:
    struct LABELREF
    {
        size_t address;
        unsigned int offsetSize;
    };

    typedef std::map<LABEL, size_t> LabelMapType;
    typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;

    void                                    WriteRexByte(bool, const CAddress&);
    void                                    WriteRexByte(bool, const CAddress&, REGISTER&);
    void                                    WriteEvOp(uint8, uint8, bool, const CAddress&);
    void                                    WriteEvGvOp(uint8, bool, const CAddress&, REGISTER);
    void                                    WriteEvIb(uint8, const CAddress&, uint8);
    void                                    WriteEvId(uint8, const CAddress&, uint32);
    void                                    WriteEvIq(uint8, const CAddress&, uint64);
    void                                    WriteEdVdOp(uint8, const CAddress&, XMMREGISTER); 
    void                                    WriteVrOp(uint8, uint8, XMMREGISTER);
    void                                    WriteStOp(uint8, uint8, uint8);

    void                                    CreateLabelReference(LABEL, unsigned int);

    static unsigned int                     GetMinimumConstantSize(uint32);
    static unsigned int                     GetMinimumConstantSize64(uint64);

    void                                    WriteByte(uint8);
    void                                    WriteDWord(uint32);
    WriteFunctionType                       m_WriteFunction;
    WriteAtFunctionType                     m_WriteAtFunction;
    TellFunctionType                        m_TellFunction;
    LabelMapType                            m_labels;
    LabelReferenceMapType                   m_labelReferences;
    LABEL                                   m_nextLabelId;
};

#endif
