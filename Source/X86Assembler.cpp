#include "X86Assembler.h"

using namespace std;

CX86Assembler::CX86Assembler(
                             const WriteFunctionType& WriteFunction,
                             const WriteAtFunctionType& WriteAtFunction,
                             const TellFunctionType& TellFunction
                             ) :
m_WriteFunction(WriteFunction),
m_WriteAtFunction(WriteAtFunction),
m_TellFunction(TellFunction),
m_nextLabelId(1)
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

CX86Assembler::CAddress CX86Assembler::MakeByteRegisterAddress(REGISTER registerId)
{
    if(registerId > 3)
    {
        throw runtime_error("Unsupported byte register index.");
    }

    return MakeRegisterAddress(registerId);
}

CX86Assembler::CAddress CX86Assembler::MakeIndRegAddress(REGISTER registerId)
{
    CAddress Address;

    if(registerId == rSP)
    {
        registerId = static_cast<REGISTER>(4);
        Address.sib.scale = 0;
        Address.sib.index = 4;
        Address.sib.base = 4;
    }
    else
    {
        assert(0);
    }

    Address.ModRm.nMod = 0;
    Address.ModRm.nRM = registerId;
    return Address;
}

CX86Assembler::CAddress CX86Assembler::MakeIndRegOffAddress(REGISTER nRegister, uint32 nOffset)
{
    CAddress Address;

    if(nRegister == rSP)
    {
        nRegister = static_cast<REGISTER>(4);
        Address.sib.scale = 0;
        Address.sib.index = 4;
        Address.sib.base = 4;
    }

    if(nRegister > 7)
    {
        Address.nIsExtendedModRM = true;
        nRegister = static_cast<REGISTER>(nRegister & 7);
    }

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

CX86Assembler::LABEL CX86Assembler::CreateLabel()
{
    return m_nextLabelId++;
}

void CX86Assembler::MarkLabel(LABEL label)
{
    m_labels[label] = m_TellFunction();
}

void CX86Assembler::ResolveLabelReferences()
{
    for(LabelReferenceMapType::iterator labelRef(m_labelReferences.begin());
        m_labelReferences.end() != labelRef; labelRef++)
    {
        LabelMapType::iterator label(m_labels.find(labelRef->first));
        if(label == m_labels.end())
        {
            throw runtime_error("Invalid label.");
        }
        size_t referencePos = labelRef->second.address;
        size_t labelPos = label->second;
        unsigned int referenceSize = labelRef->second.offsetSize;
        int offset = static_cast<int>(labelPos - referencePos - referenceSize);
        if(referenceSize == 1)
        {
            if(offset > 127 || offset < -128)
            {
                throw runtime_error("Label reference too small.");
            }
            m_WriteAtFunction(static_cast<unsigned int>(referencePos), static_cast<uint8>(offset));
        }
    }
    m_labelReferences.clear();
}

void CX86Assembler::AddEd(REGISTER registerId, const CAddress& address)
{
    WriteEvGvOp(0x03, false, address, registerId);
}

void CX86Assembler::AddId(const CAddress& Address, uint32 nConstant)
{
    WriteEvId(0x00, Address, nConstant);
}

void CX86Assembler::AndEd(REGISTER registerId, const CAddress& address)
{
    WriteEvGvOp(0x23, false, address, registerId);
}

void CX86Assembler::AndId(const CAddress& address, uint32 constant)
{
    WriteEvId(0x04, address, constant);
}

void CX86Assembler::CallEd(const CAddress& address)
{
    WriteEvOp(0xFF, 0x02, false, address);
}

void CX86Assembler::CmpEd(REGISTER registerId, const CAddress& address)
{
    WriteEvGvOp(0x3B, false, address, registerId);
}

void CX86Assembler::CmpEq(REGISTER nRegister, const CAddress& Address)
{
    WriteEvGvOp(0x3B, true, Address, nRegister);
}

void CX86Assembler::CmpId(const CAddress& address, uint32 constant)
{
    WriteEvId(0x07, address, constant);
}

void CX86Assembler::CmpIq(const CAddress& Address, uint64 nConstant)
{
    WriteEvIq(0x07, Address, nConstant);
}

void CX86Assembler::ImulEd(const CAddress& address)
{
    WriteEvOp(0xF7, 0x05, false, address);
}

void CX86Assembler::JeJb(LABEL label)
{
    WriteByte(0x74);
    CreateLabelReference(label, 1);
    WriteByte(0x00);
}

void CX86Assembler::JmpJb(LABEL label)
{
    WriteByte(0xEB);
    CreateLabelReference(label, 1);
    WriteByte(0x00);
}

void CX86Assembler::JneJb(LABEL label)
{
    WriteByte(0x75);
    CreateLabelReference(label, 1);
    WriteByte(0x00);
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

void CX86Assembler::MovsxEb(REGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEvGvOp(0xBE, false, address, registerId);
}

void CX86Assembler::MovsxEw(REGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEvGvOp(0xBF, false, address, registerId);
}

void CX86Assembler::MovzxEb(REGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEvGvOp(0xB6, false, address, registerId);
}

void CX86Assembler::Nop()
{
    WriteByte(0x90);
}

void CX86Assembler::OrEd(REGISTER registerId, const CAddress& address)
{
    WriteEvGvOp(0x0B, false, address, registerId);
}

void CX86Assembler::OrId(const CAddress& address, uint32 constant)
{
    WriteEvId(0x01, address, constant);
}

void CX86Assembler::Pop(REGISTER registerId)
{
    CAddress Address(MakeRegisterAddress(registerId));
    WriteRexByte(false, Address);
    WriteByte(0x58 | Address.ModRm.nRM);
}

void CX86Assembler::Push(REGISTER registerId)
{
    CAddress Address(MakeRegisterAddress(registerId));
    WriteRexByte(false, Address);
    WriteByte(0x50 | Address.ModRm.nRM);
}

void CX86Assembler::PushEd(const CAddress& address)
{
    WriteEvOp(0xFF, 0x06, false, address);
}

void CX86Assembler::PushId(uint32 value)
{
    WriteByte(0x68);
    WriteDWord(value);
}

void CX86Assembler::Ret()
{
    WriteByte(0xC3);
}

void CX86Assembler::SarEd(const CAddress& address, uint8 amount)
{
    WriteEvOp(0xC1, 0x07, false, address);
    WriteByte(amount);
}

void CX86Assembler::SetbEb(const CAddress& address)
{
    WriteByte(0x0F);
    WriteEvOp(0x92, 0x00, false, address);
}

void CX86Assembler::SetbeEb(const CAddress& address)
{
    WriteByte(0x0F);
    WriteEvOp(0x96, 0x00, false, address);
}

void CX86Assembler::SeteEb(const CAddress& address)
{
    WriteByte(0x0F);
    WriteEvOp(0x94, 0x00, false, address);
}

void CX86Assembler::SetlEb(const CAddress& address)
{
    WriteByte(0x0F);
    WriteEvOp(0x9C, 0x00, false, address);
}

void CX86Assembler::ShlEd(const CAddress& address, uint8 amount)
{
    WriteEvOp(0xC1, 0x04, false, address);
    WriteByte(amount);
}

void CX86Assembler::ShrEd(const CAddress& address, uint8 amount)
{
    WriteEvOp(0xC1, 0x05, false, address);
    WriteByte(amount);
}

void CX86Assembler::ShldEd(const CAddress& address, REGISTER registerId, uint8 amount)
{
    WriteByte(0x0F);
    WriteEvGvOp(0xA4, false, address, registerId);
    WriteByte(amount);
}

void CX86Assembler::ShrdEd(const CAddress& address, REGISTER registerId, uint8 amount)
{
    WriteByte(0x0F);
    WriteEvGvOp(0xAC, false, address, registerId);
    WriteByte(amount);
}

void CX86Assembler::SubEd(REGISTER nRegister, const CAddress& Address)
{
    WriteEvGvOp(0x2B, false, Address, nRegister);
}

void CX86Assembler::SubId(const CAddress& Address, uint32 nConstant)
{
    WriteEvId(0x05, Address, nConstant);
}

void CX86Assembler::TestEd(REGISTER registerId, const CAddress& address)
{
    WriteEvGvOp(0x85, false, address, registerId);
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

void CX86Assembler::WriteEvOp(uint8 opcode, uint8 subOpcode, bool is64, const CAddress& address)
{
    WriteRexByte(is64, address);
    CAddress newAddress(address);
    newAddress.ModRm.nFnReg = subOpcode;
    WriteByte(opcode);
    newAddress.Write(m_WriteFunction);
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

void CX86Assembler::CreateLabelReference(LABEL label, unsigned int size)
{
    LABELREF reference;
    reference.address = m_TellFunction();
    reference.offsetSize = size;
    m_labelReferences.insert(LabelReferenceMapType::value_type(label, reference));
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
    sib.byteValue = 0;
    nIsExtendedModRM = false;
}

void CX86Assembler::CAddress::Write(const WriteFunctionType& WriteFunction)
{
    WriteFunction(ModRm.nByte);

    if(HasSib())
    {
        WriteFunction(sib.byteValue);
    }

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

bool CX86Assembler::CAddress::HasSib() const
{
    if(ModRm.nMod == 3) return false;
    return ModRm.nRM == 4;
}
