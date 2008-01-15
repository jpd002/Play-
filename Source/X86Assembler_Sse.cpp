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

void CX86Assembler::PorVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xEB, address, registerId);
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

void CX86Assembler::PxorVo(XMMREGISTER registerId, const CAddress& address)
{
    WriteByte(0x66);
    WriteByte(0x0F);
    WriteEdVdOp(0xEF, address, registerId);
}
