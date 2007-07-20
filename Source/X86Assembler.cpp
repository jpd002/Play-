#include "X86Assembler.h"

CX86Assembler::CX86Assembler(const WriteFunctionType& WriteFunction) :
m_WriteFunction(WriteFunction)
{
    
}

CX86Assembler::~CX86Assembler()
{

}

CX86Assembler::CAddress CX86Assembler::MakeRegisterAddress(REGISTER nRegister)
{
    CAddress Address;

    if(nRegister > 7)
    {
        Address.nIsExtendedModRM = true;
        nRegister = static_cast<REGISTER>(nRegister & 7);
    }

    Address.ModRm.nMod = 3;
    Address.ModRm.nRM = nRegister;

    return Address;
}

CX86Assembler::CAddress CX86Assembler::MakeIndRegOffAddress(REGISTER nRegister, uint32 nOffset)
{
    CAddress Address;

    if(nRegister > 7)
    {
        Address.nIsExtendedModRM = true;
        nRegister = static_cast<REGISTER>(nRegister & 7);
    }

    assert(nRegister != rSP);

    if(GetMinimumConstantSize(nOffset) == 1)
    {
        Address.ModRm.nMod = 1;
        Address.nOffset = static_cast<uint8>(nOffset);
    }
    else
    {
        Address.ModRm.nMod = 2;
        Address.nOffset = nOffset;
    }

    Address.ModRm.nRM = nRegister;

    return Address;
}

void CX86Assembler::AddId(const CAddress& Address, uint32 nConstant)
{
    WriteEvId(0x00, Address, nConstant);
}

void CX86Assembler::CmpEq(REGISTER nRegister, const CAddress& Address)
{
    WriteEvGvOp(0x3B, true, Address, nRegister);
}

void CX86Assembler::CmpIq(const CAddress& Address, uint64 nConstant)
{
    WriteEvIq(0x07, Address, nConstant);
}

void CX86Assembler::Nop()
{
    WriteByte(0x90);
}

void CX86Assembler::MovEd(REGISTER nRegister, const CAddress& Address)
{
    WriteEvGvOp(0x8B, false, Address, nRegister);
}

void CX86Assembler::MovEq(REGISTER nRegister, const CAddress& Address)
{
    WriteEvGvOp(0x8B, true, Address, nRegister);
}

void CX86Assembler::MovGd(const CAddress& Address, REGISTER nRegister)
{
    WriteEvGvOp(0x89, false, Address, nRegister);
}

void CX86Assembler::MovId(REGISTER nRegister, uint32 nConstant)
{
    CAddress Address(MakeRegisterAddress(nRegister));
    WriteRexByte(false, Address);
    WriteByte(0xB8 | Address.ModRm.nRM);
    WriteDWord(nConstant);
}

void CX86Assembler::SubEd(REGISTER nRegister, const CAddress& Address)
{
    WriteEvGvOp(0x2B, false, Address, nRegister);
}

void CX86Assembler::SubId(const CAddress& Address, uint32 nConstant)
{
    WriteEvId(0x05, Address, nConstant);
}

void CX86Assembler::XorGd(const CAddress& Address, REGISTER nRegister)
{
    WriteEvGvOp(0x31, false, Address, nRegister);
}

void CX86Assembler::XorGq(const CAddress& Address, REGISTER nRegister)
{
    WriteEvGvOp(0x31, true, Address, nRegister);
}

void CX86Assembler::WriteRexByte(bool nIs64, const CAddress& Address)
{
    REGISTER nTemp = rAX;
    WriteRexByte(nIs64, Address, nTemp);
}

void CX86Assembler::WriteRexByte(bool nIs64, const CAddress& Address, REGISTER& nRegister)
{
    if((nIs64) || (Address.nIsExtendedModRM) || (nRegister > 7))
    {
        uint8 nByte = 0x40;
        nByte |= nIs64 ? 0x8 : 0x0;
        nByte |= (nRegister > 7) ? 0x04 : 0x0;
        nByte |= Address.nIsExtendedModRM ? 0x1 : 0x0;

        nRegister = static_cast<REGISTER>(nRegister & 7);

        WriteByte(nByte);
    }
}

void CX86Assembler::WriteEvGvOp(uint8 nOp, bool nIs64, const CAddress& Address, REGISTER nRegister)
{
    WriteRexByte(nIs64, Address, nRegister);
    CAddress NewAddress(Address);
    NewAddress.ModRm.nFnReg = nRegister;
    WriteByte(nOp);
    NewAddress.Write(m_WriteFunction);
}

void CX86Assembler::WriteEvId(uint8 nOp, const CAddress& Address, uint32 nConstant)
{
    //0x81 -> Id
    //0x83 -> Ib
 
    WriteRexByte(false, Address);
    CAddress NewAddress(Address);
    NewAddress.ModRm.nFnReg = nOp;

    if(GetMinimumConstantSize(nConstant) == 1)
    {
        WriteByte(0x83);
        NewAddress.Write(m_WriteFunction);
        WriteByte(static_cast<uint8>(nConstant));
    }
    else
    {
        WriteByte(0x81);
        NewAddress.Write(m_WriteFunction);
        WriteDWord(nConstant);
    }
}

void CX86Assembler::WriteEvIq(uint8 nOp, const CAddress& Address, uint64 nConstant)
{
    unsigned int nConstantSize(GetMinimumConstantSize64(nConstant));
    assert(nConstantSize <= 4);

    WriteRexByte(true, Address);
    CAddress NewAddress(Address);
    NewAddress.ModRm.nFnReg = nOp;

    if(nConstantSize == 1)
    {
        WriteByte(0x83);
        NewAddress.Write(m_WriteFunction);
        WriteByte(static_cast<uint8>(nConstant));
    }
    else
    {
        WriteByte(0x81);
        NewAddress.Write(m_WriteFunction);
        WriteDWord(static_cast<uint32>(nConstant));
    }
}

unsigned int CX86Assembler::GetMinimumConstantSize(uint32 nConstant)
{
	if((static_cast<int32>(nConstant) >= -128) && (static_cast<int32>(nConstant) <= 127))
	{
		return 1;
	}
	return 4;
}

unsigned int CX86Assembler::GetMinimumConstantSize64(uint64 nConstant)
{
    if((static_cast<int64>(nConstant) >= -128) && (static_cast<int64>(nConstant) <= 127))
    {
        return 1;
    }
    if((static_cast<int64>(nConstant) >= -2147483647) && (static_cast<int64>(nConstant) <= 2147483647))
    {
        return 4;
    }
    return 8;
}

void CX86Assembler::WriteByte(uint8 nByte)
{
    m_WriteFunction(nByte);
}

void CX86Assembler::WriteDWord(uint32 nDWord)
{
    uint8* pByte(reinterpret_cast<uint8*>(&nDWord));
    m_WriteFunction(pByte[0]);
    m_WriteFunction(pByte[1]);
    m_WriteFunction(pByte[2]);
    m_WriteFunction(pByte[3]);
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

CX86Assembler::CAddress::CAddress()
{
    ModRm.nByte = 0;
    nIsExtendedModRM = false;
}

void CX86Assembler::CAddress::Write(const WriteFunctionType& WriteFunction)
{
    WriteFunction(ModRm.nByte);

    if(ModRm.nMod == 1)
    {
        WriteFunction(static_cast<uint8>(nOffset));
    }
    else if(ModRm.nMod == 2)
    {
        uint8* pByte(reinterpret_cast<uint8*>(&nOffset));
        WriteFunction(pByte[0]);
        WriteFunction(pByte[1]);
        WriteFunction(pByte[2]);
        WriteFunction(pByte[3]);
    }
}
