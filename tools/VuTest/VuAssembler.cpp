#include "VuAssembler.h"
#include <cassert>
#include <stdexcept>

#define BRANCH_MIN -1024
#define BRANCH_MAX 1023

CVuAssembler::CVuAssembler(uint32* ptr)
    : m_ptr(ptr)
    , m_startPtr(ptr)
{
}

CVuAssembler::~CVuAssembler()
{
	ResolveLabelReferences();
}

unsigned int CVuAssembler::GetProgramSize() const
{
	auto wordSize = static_cast<unsigned int>(m_ptr - m_startPtr);
	assert((wordSize & 1) == 0);
	return wordSize / 2;
}

CVuAssembler::LABEL CVuAssembler::CreateLabel()
{
	return m_nextLabelId++;
}

void CVuAssembler::MarkLabel(LABEL label)
{
	m_labels[label] = GetProgramSize();
}

void CVuAssembler::CreateLabelReference(LABEL label)
{
	LABELREF reference;
	reference.address = GetProgramSize();
	m_labelReferences.insert(LabelReferenceMapType::value_type(label, reference));
}

void CVuAssembler::ResolveLabelReferences()
{
	for(const auto& labelRef : m_labelReferences)
	{
		auto label(m_labels.find(labelRef.first));
		if(label == m_labels.end())
		{
			throw std::runtime_error("Invalid label.");
		}
		size_t referencePos = labelRef.second.address;
		size_t labelPos = label->second;
		int offset = static_cast<int>(labelPos - referencePos - 1);
		if(offset > BRANCH_MAX || offset < BRANCH_MIN)
		{
			throw std::runtime_error("Jump length too long.");
		}
		uint32& instruction = m_startPtr[referencePos * 2];
		instruction &= 0xFFFFF800;
		instruction |= static_cast<uint32>(offset & 0x7FF);
	}
	m_labelReferences.clear();
}

void CVuAssembler::Write(uint32 upperOp, uint32 lowerOp)
{
	m_ptr[0] = lowerOp;
	m_ptr[1] = upperOp;
	m_ptr += 2;
}

void CVuAssembler::Write(uint32 upperOp, BRANCHOP lowerOp)
{
	CreateLabelReference(lowerOp.label);
	m_ptr[0] = lowerOp.op;
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

CVuAssembler::BRANCHOP CVuAssembler::Lower::B(LABEL label)
{
	BRANCHOP result;
	result.label = label;
	result.op = 0x40000000;
	return result;
}

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

CVuAssembler::BRANCHOP CVuAssembler::Lower::IBEQ(VI_REGISTER it, VI_REGISTER is, LABEL label)
{
	BRANCHOP result;
	result.label = label;
	result.op = 0x50000000;
	result.op |= (it << 16);
	result.op |= (is << 11);
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
