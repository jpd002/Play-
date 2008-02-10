#include "X86Assembler.h"

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

void CX86Assembler::Packuswb(XMMREGISTER registerId, const CAddress& address)
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

void CX86Assembler::PandVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xDB, address, registerId);
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

void CX86Assembler::PsrawVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x71, 0x04, registerId);
    WriteByte(amount);
}

void CX86Assembler::PsrlwVo(XMMREGISTER registerId, uint8 amount)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteVrOp(0x71, 0x02, registerId);
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

void CX86Assembler::PunpckhbwVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0x68, address, registerId);
}

void CX86Assembler::PxorVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xEF, address, registerId);
}
