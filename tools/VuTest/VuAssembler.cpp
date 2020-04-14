#include "VuAssembler.h"
#include <cassert>

CVuAssembler::CVuAssembler(uint32* ptr)
    : m_ptr(ptr)
{
}

CVuAssembler::~CVuAssembler()
{
}

void CVuAssembler::Write(uint32 upperOp, uint32 lowerOp)
{
	m_ptr[0] = lowerOp;
	m_ptr[1] = upperOp;
	m_ptr += 2;
}

//---------------------------------------------------------------------------------
//UPPER OPs
//---------------------------------------------------------------------------------

uint32 CVuAssembler::Upper::ADDi(DEST dest, VF_REGISTER fd, VF_REGISTER fs)
{
	uint32 result = 0x00000022;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::CLIP(VF_REGISTER fs, VF_REGISTER ft)
{
	uint32 result = 0x01E001FF;
	result |= (fs << 11);
	result |= (ft << 16);
	return result;
}

uint32 CVuAssembler::Upper::FTOI4(DEST dest, VF_REGISTER ft, VF_REGISTER fs)
{
	uint32 result = 0x0000017D;
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::ITOF0(DEST dest, VF_REGISTER ft, VF_REGISTER fs)
{
	uint32 result = 0x0000013C;
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::ITOF12(DEST dest, VF_REGISTER ft, VF_REGISTER fs)
{
	uint32 result = 0x0000013E;
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::MADDbc(DEST dest, VF_REGISTER fd, VF_REGISTER fs, VF_REGISTER ft, BROADCAST bc)
{
	uint32 result = 0x00000008;
	result |= bc;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::MADDAbc(DEST dest, VF_REGISTER fs, VF_REGISTER ft, BROADCAST bc)
{
	uint32 result = 0x000000BC;
	result |= bc;
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::MULi(DEST dest, VF_REGISTER fd, VF_REGISTER fs)
{
	uint32 result = 0x0000001E;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::MULq(DEST dest, VF_REGISTER fd, VF_REGISTER fs)
{
	uint32 result = 0x0000001C;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::MULAbc(DEST dest, VF_REGISTER fs, VF_REGISTER ft, BROADCAST bc)
{
	uint32 result = 0x000001BC;
	result |= bc;
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::MAX(DEST dest, VF_REGISTER fd, VF_REGISTER fs, VF_REGISTER ft)
{
	uint32 result = 0x0000002B;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::MINI(DEST dest, VF_REGISTER fd, VF_REGISTER fs, VF_REGISTER ft)
{
	uint32 result = 0x0000002F;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Upper::NOP()
{
	return 0x000002FF;
}

uint32 CVuAssembler::Upper::OPMULA(VF_REGISTER fs, VF_REGISTER ft)
{
	uint32 result = 0x01C002FE;
	result |= (fs << 11);
	result |= (ft << 16);
	return result;
}

uint32 CVuAssembler::Upper::OPMSUB(VF_REGISTER fd, VF_REGISTER fs, VF_REGISTER ft)
{
	uint32 result = 0x01C0002E;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (ft << 16);
	return result;
}

uint32 CVuAssembler::Upper::SUBbc(DEST dest, VF_REGISTER fd, VF_REGISTER fs, VF_REGISTER ft, BROADCAST bc)
{
	uint32 result = 0x00000004;
	result |= bc;
	result |= (fd << 6);
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

//---------------------------------------------------------------------------------
//LOWER OPs
//---------------------------------------------------------------------------------

uint32 CVuAssembler::Lower::DIV(VF_REGISTER fs, FVF fsf, VF_REGISTER ft, FVF ftf)
{
	uint32 result = 0x800003BC;
	result |= (ftf << 23);
	result |= (fsf << 21);
	result |= (ft << 16);
	result |= (fs << 11);
	return result;
}

uint32 CVuAssembler::Lower::FCAND(uint32 imm)
{
	uint32 result = 0x24000000;
	result |= (imm & 0xFFFFFF);
	return result;
}

uint32 CVuAssembler::Lower::FMAND(VI_REGISTER it, VI_REGISTER is)
{
	uint32 result = 0x34000000;
	result |= (it << 16);
	result |= (is << 11);
	return result;
}

uint32 CVuAssembler::Lower::FSAND(VI_REGISTER it, uint16 imm)
{
	imm &= 0xFFF;
	uint32 result = 0x2C000000;
	result |= (it << 16);
	result |= (imm & 0x7FF);
	result |= (((imm & 0x800) >> 11) << 21);
	return result;
}

uint32 CVuAssembler::Lower::IADDIU(VI_REGISTER it, VI_REGISTER is, uint16 imm)
{
	assert(imm <= 0x7FFF);
	imm &= 0x7FFF;
	uint32 result = 0x10000000;
	result |= (it << 16);
	result |= (is << 11);
	result |= (imm & 0x7FF);
	result |= (((imm & 0x7800) >> 11) << 21);
	return result;
}

uint32 CVuAssembler::Lower::LQ(DEST dest, VF_REGISTER ft, uint16 imm, VI_REGISTER is)
{
	uint32 result = 0x00000000;
	result |= (imm & 0x7FF);
	result |= (is << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Lower::NOP()
{
	return 0x8000033C;
}

uint32 CVuAssembler::Lower::SQ(DEST dest, VF_REGISTER fs, uint16 imm, VI_REGISTER it)
{
	uint32 result = 0x02000000;
	result |= (imm & 0x7FF);
	result |= (fs << 11);
	result |= (it << 16);
	result |= (dest << 21);
	return result;
}

uint32 CVuAssembler::Lower::WAITQ()
{
	return 0x800003BF;
}
