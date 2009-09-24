#include "X86Assembler.h"

//------------------------------------------------
//Scalar Instructions
//------------------------------------------------

void CX86Assembler::MovssEd(const CAddress& address, XMMREGISTER registerId)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x11, address, registerId);
}

void CX86Assembler::MovssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x10, address, registerId);
}

void CX86Assembler::AddssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x58, address, registerId);
}

void CX86Assembler::SubssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x5C, address, registerId);
}

void CX86Assembler::MaxssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x5F, address, registerId);
}

void CX86Assembler::MinssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x5D, address, registerId);
}

void CX86Assembler::MulssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x59, address, registerId);
}

void CX86Assembler::DivssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x5E, address, registerId);
}

void CX86Assembler::RcpssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x53, address, registerId);
}

void CX86Assembler::RsqrtssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x52, address, registerId);
}

void CX86Assembler::SqrtssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x51, address, registerId);
}

void CX86Assembler::CmpssEd(XMMREGISTER registerId, const CAddress& address, SSE_CMP_TYPE condition)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0xC2, address, registerId);
    WriteByte(static_cast<uint8>(condition));
}

void CX86Assembler::Cvtsi2ssEd(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x2A, address, registerId);
}

void CX86Assembler::Cvttss2siEd(REGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEvGvOp(0x2C, false, address, registerId);
}

//------------------------------------------------
//Packed Instructions
//------------------------------------------------

void CX86Assembler::MovdVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x6E, address, registerId);
}

void CX86Assembler::MovdquVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x6F, address, registerId);
}

void CX86Assembler::MovdquVo(const CAddress& address, XMMREGISTER registerId)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x7F, address, registerId);
}

void CX86Assembler::MovapsVo(const CAddress& address, XMMREGISTER registerId)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x29, address, registerId);
}

void CX86Assembler::MovapsVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x28, address, registerId);
}

void CX86Assembler::PackssdwVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x6B, address, registerId);
}

void CX86Assembler::PackuswbVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x67, address, registerId);
}

void CX86Assembler::PaddwVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xFD, address, registerId);
}

void CX86Assembler::PadddVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xFE, address, registerId);
}

void CX86Assembler::PandVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xDB, address, registerId);
}

void CX86Assembler::PandnVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xDF, address, registerId);
}

void CX86Assembler::PcmpeqdVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x76, address, registerId);
}

void CX86Assembler::PcmpgtwVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x65, address, registerId);
}

void CX86Assembler::PmaxswVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xEE, address, registerId);
}

void CX86Assembler::PminswVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xEA, address, registerId);
}

void CX86Assembler::PorVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xEB, address, registerId);
}

void CX86Assembler::PsllwVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x71, 0x06, registerId);
    WriteByte(amount);
}

void CX86Assembler::PslldVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x72, 0x06, registerId);
    WriteByte(amount);
}

void CX86Assembler::PsrawVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x71, 0x04, registerId);
    WriteByte(amount);
}

void CX86Assembler::PsradVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x72, 0x04, registerId);
    WriteByte(amount);
}

void CX86Assembler::PsrlwVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x71, 0x02, registerId);
    WriteByte(amount);
}

void CX86Assembler::PsrldVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x72, 0x02, registerId);
    WriteByte(amount);
}

void CX86Assembler::PsubbVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xF8, address, registerId);
}

void CX86Assembler::PsubdVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xFA, address, registerId);
}

void CX86Assembler::PunpcklbwVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x60, address, registerId);
}

void CX86Assembler::PunpcklwdVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x61, address, registerId);
}

void CX86Assembler::PunpckldqVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x62, address, registerId);
}

void CX86Assembler::PunpckhbwVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x68, address, registerId);
}

void CX86Assembler::PunpckhdqVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x6A, address, registerId);
}

void CX86Assembler::PxorVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xEF, address, registerId);
}

void CX86Assembler::AddpsVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x58, address, registerId);
}

void CX86Assembler::DivpsVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x5E, address, registerId);
}

void CX86Assembler::Cvtdq2psVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x5B, address, registerId);
}

void CX86Assembler::Cvttps2dqVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0xF3);
    WriteByte(0x0F);
    WriteEdVdOp(0x5B, address, registerId);
}

void CX86Assembler::MaxpsVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x5F, address, registerId);
}

void CX86Assembler::MinpsVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x5D, address, registerId);
}

void CX86Assembler::MulpsVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x59, address, registerId);
}

void CX86Assembler::SubpsVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x0F);
    WriteEdVdOp(0x5C, address, registerId);
}

void CX86Assembler::ShufpsVo(XMMREGISTER registerId, const CAddress& address, uint8 shuffleByte)
{
    WriteByte(0x0F);
    WriteEdVdOp(0xC6, address, registerId);
    WriteByte(shuffleByte);
}

//------------------------------------------------
//Addressing utils
//------------------------------------------------

void CX86Assembler::WriteEdVdOp(uint8 opcode, const CAddress& address, XMMREGISTER xmmRegisterId)
{
    REGISTER registerId = static_cast<REGISTER>(xmmRegisterId);
    WriteRexByte(false, address, registerId);
    CAddress NewAddress(address);
    NewAddress.ModRm.nFnReg = registerId;
    WriteByte(opcode);
    NewAddress.Write(m_WriteFunction);
}

void CX86Assembler::WriteVrOp(uint8 opcode, uint8 subOpcode, XMMREGISTER registerId)
{
    CAddress address(MakeXmmRegisterAddress(registerId));
    WriteRexByte(false, address);
    address.ModRm.nFnReg = subOpcode;
    WriteByte(opcode);
    address.Write(m_WriteFunction);
}
