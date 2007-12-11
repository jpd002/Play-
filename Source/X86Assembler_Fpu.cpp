#include "X86Assembler.h"

using namespace std;

void CX86Assembler::FldEd(const CAddress& address)
{
    if(address.ModRm.nMod == 3) throw runtime_error("Invalid address mod.");
    WriteEvOp(0xD9, 0x00, false, address);
}

void CX86Assembler::FildEd(const CAddress& address)
{
    if(address.ModRm.nMod == 3) throw runtime_error("Invalid address mod.");
    WriteEvOp(0xDB, 0x00, false, address);
}

void CX86Assembler::FstpEd(const CAddress& address)
{
    if(address.ModRm.nMod == 3) throw runtime_error("Invalid address mod.");
    WriteEvOp(0xD9, 0x03, false, address);
}

void CX86Assembler::FistpEd(const CAddress& address)
{
    if(address.ModRm.nMod == 3) throw runtime_error("Invalid address mod.");
    WriteEvOp(0xDB, 0x03, false, address);
}

void CX86Assembler::FisttpEd(const CAddress& address)
{
    if(address.ModRm.nMod == 3) throw runtime_error("Invalid address mod.");
    WriteEvOp(0xDB, 0x01, false, address);
}

void CX86Assembler::FaddpSt(uint8 stackId)
{
    WriteStOp(0xDE, 0x00, stackId);
}

void CX86Assembler::FsubpSt(uint8 stackId)
{
    WriteStOp(0xDE, 0x05, stackId);
}

void CX86Assembler::FmulpSt(uint8 stackId)
{
    WriteStOp(0xDE, 0x01, stackId);
}

void CX86Assembler::FdivpSt(uint8 stackId)
{
    WriteStOp(0xDE, 0x07, stackId); 
}

void CX86Assembler::Fwait()
{
    WriteByte(0x9B);
}

void CX86Assembler::FnstcwEw(const CAddress& address)
{
    if(address.ModRm.nMod == 3) throw runtime_error("Invalid address mod.");
    WriteEvOp(0xD9, 0x07, false, address);
}

void CX86Assembler::FldcwEw(const CAddress& address)
{
    if(address.ModRm.nMod == 3) throw runtime_error("Invalid address mod.");
    WriteEvOp(0xD9, 0x05, false, address);
}

void CX86Assembler::WriteStOp(uint8 opcode, uint8 subOpcode, uint8 stackId)
{
    CAddress address;
    address.ModRm.nMod = 3;
    address.ModRm.nFnReg = subOpcode;
    address.ModRm.nRM = stackId;
    WriteByte(opcode);
    WriteByte(address.ModRm.nByte);
}
