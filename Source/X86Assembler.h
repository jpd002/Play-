#ifndef _X86ASSEMBLER_H_
#define _X86ASSEMBLER_H_

#include "Types.h"
#include <boost/function.hpp>

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

    typedef boost::function<void (uint8)>   WriteFunctionType;

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

                                            CX86Assembler(const WriteFunctionType&);
    virtual                                 ~CX86Assembler();

    static CAddress                         MakeRegisterAddress(REGISTER);
    static CAddress                         MakeIndRegOffAddress(REGISTER, uint32);

    void                                    AddId(const CAddress&, uint32);
    void                                    CmpEq(REGISTER, const CAddress&);
    void                                    CmpId(const CAddress&, uint32);
    void                                    CmpIq(const CAddress&, uint64);
    void                                    Nop();
    void                                    MovEd(REGISTER, const CAddress&);
    void                                    MovEq(REGISTER, const CAddress&);
    void                                    MovGd(const CAddress&, REGISTER);
    void                                    MovId(REGISTER, uint32);
    void                                    SubEd(REGISTER, const CAddress&);
    void                                    SubId(const CAddress&, uint32);
    void                                    XorGd(const CAddress&, REGISTER);
    void                                    XorGq(const CAddress&, REGISTER);

private:
    void                                    WriteRexByte(bool, const CAddress&);
    void                                    WriteRexByte(bool, const CAddress&, REGISTER&);
    void                                    WriteEvGvOp(uint8, bool, const CAddress&, REGISTER);
    void                                    WriteEvId(uint8, const CAddress&, uint32);
    void                                    WriteEvIq(uint8, const CAddress&, uint64);

    static unsigned int                     GetMinimumConstantSize(uint32);
    static unsigned int                     GetMinimumConstantSize64(uint64);

    void                                    WriteByte(uint8);
    void                                    WriteDWord(uint32);
    WriteFunctionType                       m_WriteFunction;
};

#endif
