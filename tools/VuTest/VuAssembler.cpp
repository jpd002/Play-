#include "VuAssembler.h"

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
	result |= (fd <<  6);
	result |= (fs << 11);
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

uint32 CVuAssembler::Upper::MADDbc(DEST dest, VF_REGISTER fd, VF_REGISTER fs, VF_REGISTER ft, BROADCAST bc)
{
	uint32 result = 0x00000008;
	result |= bc;
	result |= (fd <<  6);
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
	result |= (fd <<  6);
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
	result |= (fd <<  6);
	result |= (fs << 11);
	result |= (ft << 16);
	return result;
}

uint32 CVuAssembler::Upper::SUBbc(DEST dest, VF_REGISTER fd, VF_REGISTER fs, VF_REGISTER ft, BROADCAST bc)
{
	uint32 result = 0x00000004;
	result |= bc;
	result |= (fd <<  6);
	result |= (fs << 11);
	result |= (ft << 16);
	result |= (dest << 21);
	return result;
}

//---------------------------------------------------------------------------------
//LOWER OPs
//---------------------------------------------------------------------------------

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

uint32 CVuAssembler::Lower::NOP()
{
	return 0x8000033C;
}
