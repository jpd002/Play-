#include "MIPSAssembler.h"
#include "MIPS.h"
#include <limits.h>
#include "lexical_cast_ex.h"

CMIPSAssembler::CMIPSAssembler(uint32* ptr)
: m_ptr(ptr)
, m_startPtr(ptr)
, m_nextLabelId(1)
{

}

CMIPSAssembler::~CMIPSAssembler()
{
	ResolveLabelReferences();
}

unsigned int CMIPSAssembler::GetProgramSize()
{
	return static_cast<unsigned int>(m_ptr - m_startPtr);
}

CMIPSAssembler::LABEL CMIPSAssembler::CreateLabel()
{
	LABEL newLabel;
	newLabel.id = m_nextLabelId++;
	return newLabel;
}

void CMIPSAssembler::MarkLabel(LABEL label)
{
	m_labels[label] = GetProgramSize();
}

void CMIPSAssembler::CreateLabelReference(LABEL label)
{
	LABELREF reference;
	reference.address = GetProgramSize();
	m_labelReferences.insert(LabelReferenceMapType::value_type(label, reference));
}

void CMIPSAssembler::ResolveLabelReferences()
{
	for(const auto& labelRef : m_labelReferences)
	{
		LabelMapType::iterator label(m_labels.find(labelRef.first));
		if(label == m_labels.end())
		{
			throw std::runtime_error("Invalid label.");
		}
		size_t referencePos = labelRef.second.address;
		size_t labelPos = label->second;
		int offset = static_cast<int>(labelPos - referencePos - 1);
		if(offset > SHRT_MAX || offset < SHRT_MIN)
		{
			throw std::runtime_error("Jump length too long.");
		}
		uint32& instruction = m_startPtr[referencePos];
		instruction &= 0xFFFF0000;
		instruction |= static_cast<uint16>(offset);
	}
	m_labelReferences.clear();
}

void CMIPSAssembler::ADDIU(unsigned int rt, unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x09) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::ADDU(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21)  | (rt << 16) | (rd << 11) | (0x21);
	m_ptr++;
}

void CMIPSAssembler::AND(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21)  | (rt << 16) | (rd << 11) | (0x24);
	m_ptr++;
}

void CMIPSAssembler::ANDI(unsigned int rt, unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x0C) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::BEQ(unsigned int rs, unsigned int rt, uint16 immediate)
{
	(*m_ptr) = ((0x04) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::BEQ(unsigned int rs, unsigned int rt, LABEL label)
{
	CreateLabelReference(label);
	BEQ(rs, rt, 0);
}

void CMIPSAssembler::BGEZ(unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x01) << 26) | (rs << 21) | ((0x01) << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::BGEZ(unsigned int rs, LABEL label)
{
	CreateLabelReference(label);
	BGEZ(rs, 0);
}

void CMIPSAssembler::BGTZ(unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x07) << 26) | (rs << 21) | immediate;
	m_ptr++;
}

void CMIPSAssembler::BNE(unsigned int rs, unsigned int rt, uint16 immediate)
{
	(*m_ptr) = ((0x05) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::BNE(unsigned int rs, unsigned int rt, LABEL label)
{
	CreateLabelReference(label);
	BNE(rs, rt, 0);
}

void CMIPSAssembler::BLEZ(unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x06) << 26) | (rs << 21) | immediate;
	m_ptr++;
}

void CMIPSAssembler::BLTZ(unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x01) << 26) | (rs << 21) | immediate;
	m_ptr++;
}

void CMIPSAssembler::DADDU(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | (0x2D);
	m_ptr++;
}

void CMIPSAssembler::DADDIU(unsigned int rt, unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x19) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::DSLL(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x38;
	m_ptr++;
}

void CMIPSAssembler::DSLL32(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x3C;
	m_ptr++;
}

void CMIPSAssembler::DSLLV(unsigned int rd, unsigned int rt, unsigned int rs)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x14;
	m_ptr++;
}

void CMIPSAssembler::DSRA(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x3B;
	m_ptr++;
}

void CMIPSAssembler::DSRA32(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x3F;
	m_ptr++;
}

void CMIPSAssembler::DSRAV(unsigned int rd, unsigned int rt, unsigned int rs)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x17;
	m_ptr++;
}

void CMIPSAssembler::DSRL(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x3A;
	m_ptr++;
}

void CMIPSAssembler::DSRL32(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x3E;
	m_ptr++;
}

void CMIPSAssembler::DSRLV(unsigned int rd, unsigned int rt, unsigned int rs)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x16;
	m_ptr++;
}

void CMIPSAssembler::DSUBU(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | (0x2F);
	m_ptr++;
}

void CMIPSAssembler::ERET()
{
	(*m_ptr) = 0x42000018;
	m_ptr++;
}

void CMIPSAssembler::JAL(uint32 address)
{
	(*m_ptr) = ((0x03) << 26) | ((address >> 2) & 0x03FFFFFF);
	m_ptr++;
}

void CMIPSAssembler::JALR(unsigned int rs, unsigned int rd)
{
	(*m_ptr) = (rs << 21) | (rd << 11) | (0x09);
	m_ptr++;
}

void CMIPSAssembler::JR(unsigned int rs)
{
	(*m_ptr) = (rs << 21) | (0x08);
	m_ptr++;
}

void CMIPSAssembler::LD(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x37) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::LDL(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x1A) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::LDR(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x1B) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::LHU(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x25) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::LI(unsigned int rt, uint32 immediate)
{
	uint16 low = static_cast<int16>(immediate);
	uint16 high = static_cast<int16>(immediate >> 16);
	LUI(rt, high);
	if(low != 0)
	{
		ORI(rt, rt, low);
	}
}

void CMIPSAssembler::LUI(unsigned int rt, uint16 immediate)
{
	(*m_ptr) = ((0x0F) << 26) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::LBU(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x24) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::LW(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x23) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::LWL(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x22) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::LWR(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x26) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::MFC0(unsigned int rt, unsigned int rd)
{
	(*m_ptr) = ((0x10) << 26) | (rt << 16) | (rd << 11);
	m_ptr++;
}

void CMIPSAssembler::MFHI(unsigned int rd)
{
	(*m_ptr) = ((0x00) << 26) | (rd << 11) | (0x10);
	m_ptr++;
}

void CMIPSAssembler::MFLO(unsigned int rd)
{
	(*m_ptr) = ((0x00) << 26) | (rd << 11) | (0x12);
	m_ptr++;
}

void CMIPSAssembler::MTC0(unsigned int rt, unsigned int rd)
{
	(*m_ptr) = ((0x10) << 26) | ((0x04) << 21) | (rt << 16) | (rd << 11);
	m_ptr++;
}

void CMIPSAssembler::MTHI(unsigned int rs)
{
	(*m_ptr) = ((0x00) << 26) | (rs << 21) | (0x11);
	m_ptr++;
}

void CMIPSAssembler::MTLO(unsigned int rs)
{
	(*m_ptr) = ((0x00) << 26) | (rs << 21) | (0x13);
	m_ptr++;
}

void CMIPSAssembler::MOV(unsigned int rd, unsigned int rs)
{
	ADDU(rd, rs, 0);
}

void CMIPSAssembler::MULT(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x18;
	m_ptr++;
}

void CMIPSAssembler::MULTU(unsigned int rs, unsigned int rt, unsigned int rd)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x19;
	m_ptr++;
}

void CMIPSAssembler::NOP()
{
	(*m_ptr) = 0;
	m_ptr++;
}

void CMIPSAssembler::NOR(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x27;
	m_ptr++;
}

void CMIPSAssembler::OR(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x25;
	m_ptr++;
}

void CMIPSAssembler::ORI(unsigned int rt, unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x0D) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::SD(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x3F) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::SLL(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6);
	m_ptr++;
}

void CMIPSAssembler::SLLV(unsigned int rd, unsigned int rt, unsigned int rs)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x04;
	m_ptr++;
}

void CMIPSAssembler::SLT(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x2A;
	m_ptr++;
}

void CMIPSAssembler::SLTI(unsigned int rt, unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x0A) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::SLTIU(unsigned int rt, unsigned int rs, uint16 immediate)
{
	(*m_ptr) = ((0x0B) << 26) | (rs << 21) | (rt << 16) | immediate;
	m_ptr++;
}

void CMIPSAssembler::SLTU(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x2B;
	m_ptr++;
}

void CMIPSAssembler::SRA(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x03;
	m_ptr++;
}

void CMIPSAssembler::SRAV(unsigned int rd, unsigned int rt, unsigned int rs)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x07;
	m_ptr++;
}

void CMIPSAssembler::SRL(unsigned int rd, unsigned int rt, unsigned int sa)
{
	sa &= 0x1F;
	(*m_ptr) = (rt << 16) | (rd << 11) | (sa << 6) | 0x02;
	m_ptr++;
}

void CMIPSAssembler::SRLV(unsigned int rd, unsigned int rt, unsigned int rs)
{
	(*m_ptr) = (rs << 21) | (rt << 16) | (rd << 11) | 0x06;
	m_ptr++;
}

void CMIPSAssembler::SB(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x28) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::SW(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x2B) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CMIPSAssembler::SYSCALL()
{
	(*m_ptr) = 0x0000000C;
	m_ptr++;
}
