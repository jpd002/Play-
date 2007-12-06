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

    typedef std::tr1::function<void (uint8)>                WriteFunctionType;
    typedef std::tr1::function<void (unsigned int, uint8)>  WriteAtFunctionType;
    typedef std::tr1::function<size_t ()>                   TellFunctionType;
    typedef unsigned int LABEL;

    class CAddress
    {
    public:
                        CAddress();

        bool            nIsExtendedModRM;

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

        MODRMBYTE       ModRm;
        uint32          nOffset;

        void            Write(const WriteFunctionType&);
    };

                                            CX86Assembler(
                                                const WriteFunctionType&, 
                                                const WriteAtFunctionType&,
                                                const TellFunctionType&);
    virtual                                 ~CX86Assembler();

    static CAddress                         MakeRegisterAddress(REGISTER);
    static CAddress                         MakeByteRegisterAddress(REGISTER);
    static CAddress                         MakeIndRegOffAddress(REGISTER, uint32);

    LABEL                                   CreateLabel();
    void                                    MarkLabel(LABEL);
    void                                    ResolveLabelReferences();

    void                                    AddEd(REGISTER, const CAddress&);
    void                                    AddId(const CAddress&, uint32);
    void                                    AndEd(REGISTER, const CAddress&);
    void                                    AndId(const CAddress&, uint32);
    void                                    CallEd(const CAddress&);
    void                                    CmpEd(REGISTER, const CAddress&);
    void                                    CmpEq(REGISTER, const CAddress&);
    void                                    CmpId(const CAddress&, uint32);
    void                                    CmpIq(const CAddress&, uint64);
    void                                    ImulEd(const CAddress&);
    void                                    JeJb(LABEL);
    void                                    JmpJb(LABEL);
    void                                    JneJb(LABEL);
    void                                    MovEd(REGISTER, const CAddress&);
    void                                    MovEq(REGISTER, const CAddress&);
    void                                    MovGd(const CAddress&, REGISTER);
    void                                    MovId(REGISTER, uint32);
    void                                    MovzxEb(REGISTER, const CAddress&);
    void                                    Nop();
    void                                    OrEd(REGISTER, const CAddress&);
    void                                    OrId(const CAddress&, uint32);
    void                                    Push(REGISTER);
    void                                    PushEd(const CAddress&);
    void                                    PushId(uint32);
    void                                    Ret();
    void                                    SarEd(const CAddress&, uint8);
    void                                    SetbEb(const CAddress&);
    void                                    SetbeEb(const CAddress&);
    void                                    SeteEb(const CAddress&);
    void                                    SetlEb(const CAddress&);
    void                                    ShrEd(const CAddress&, uint8);
    void                                    ShrdEd(const CAddress&, REGISTER, uint8);
    void                                    ShlEd(const CAddress&, uint8);
    void                                    ShldEd(const CAddress&, REGISTER, uint8);
    void                                    SubEd(REGISTER, const CAddress&);
    void                                    SubId(const CAddress&, uint32);
    void                                    TestEd(REGISTER, const CAddress&);
    void                                    XorGd(const CAddress&, REGISTER);
    void                                    XorGq(const CAddress&, REGISTER);

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
    void                                    WriteEvId(uint8, const CAddress&, uint32);
    void                                    WriteEvIq(uint8, const CAddress&, uint64);

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
