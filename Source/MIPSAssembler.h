#pragma once

#include "Types.h"
#include <map>

class CMIPSAssembler
{
public:
	struct LABEL
	{
		unsigned int id;

		bool operator<(const LABEL& rhs) const
		{
			return id < rhs.id;
		};
	};

	CMIPSAssembler(uint32*);
	~CMIPSAssembler();

	unsigned int GetProgramSize();
	LABEL CreateLabel();
	void MarkLabel(LABEL);

	void ADDIU(unsigned int, unsigned int, uint16);
	void ADDU(unsigned int, unsigned int, unsigned int);
	void AND(unsigned int, unsigned int, unsigned int);
	void ANDI(unsigned int, unsigned int, uint16);
	void BEQ(unsigned int, unsigned int, uint16);
	void BEQ(unsigned int, unsigned int, LABEL);
	void BGEZ(unsigned int, uint16);
	void BGEZ(unsigned int, LABEL);
	void BGTZ(unsigned int, uint16);
	void BNE(unsigned int, unsigned int, uint16);
	void BNE(unsigned int, unsigned int, LABEL);
	void BLEZ(unsigned int, uint16);
	void BLTZ(unsigned int, uint16);
	void DADDU(unsigned int, unsigned int, unsigned int);
	void DADDIU(unsigned int, unsigned int, uint16);
	void DSLL(unsigned int, unsigned int, unsigned int);
	void DSLL32(unsigned int, unsigned int, unsigned int);
	void DSLLV(unsigned int, unsigned int, unsigned int);
	void DSRA(unsigned int, unsigned int, unsigned int);
	void DSRA32(unsigned int, unsigned int, unsigned int);
	void DSRAV(unsigned int, unsigned int, unsigned int);
	void DSRL(unsigned int, unsigned int, unsigned int);
	void DSRL32(unsigned int, unsigned int, unsigned int);
	void DSRLV(unsigned int, unsigned int, unsigned int);
	void DSUBU(unsigned int, unsigned int, unsigned int);
	void ERET();
	void JR(unsigned int);
	void JAL(uint32);
	void JALR(unsigned int, unsigned int = 31);
	void LBU(unsigned int, uint16, unsigned int);
	void LD(unsigned int, uint16, unsigned int);
	void LDL(unsigned int, uint16, unsigned int);
	void LDR(unsigned int, uint16, unsigned int);
	void LHU(unsigned int, uint16, unsigned int);
	void LI(unsigned int, uint32);
	void LUI(unsigned int, uint16);
	void LW(unsigned int, uint16, unsigned int);
	void LWL(unsigned int, uint16, unsigned int);
	void LWR(unsigned int, uint16, unsigned int);
	void MFC0(unsigned int, unsigned int);
	void MFHI(unsigned int);
	void MFLO(unsigned int);
	void MTC0(unsigned int, unsigned int);
	void MTHI(unsigned int);
	void MTLO(unsigned int);
	void MOV(unsigned int, unsigned int);
	void MULT(unsigned int, unsigned int, unsigned int);
	void MULTU(unsigned int, unsigned int, unsigned int);
	void NOP();
	void NOR(unsigned int, unsigned int, unsigned int);
	void OR(unsigned int, unsigned int, unsigned int);
	void ORI(unsigned int, unsigned int, uint16);
	void SD(unsigned int, uint16, unsigned int);
	void SLL(unsigned int, unsigned int, unsigned int);
	void SLLV(unsigned int, unsigned int, unsigned int);
	void SLT(unsigned int, unsigned int, unsigned int);
	void SLTI(unsigned int, unsigned int, uint16);
	void SLTIU(unsigned int, unsigned int, uint16);
	void SLTU(unsigned int, unsigned int, unsigned int);
	void SRA(unsigned int, unsigned int, unsigned int);
	void SRAV(unsigned int, unsigned int, unsigned int);
	void SRL(unsigned int, unsigned int, unsigned int);
	void SRLV(unsigned int, unsigned int, unsigned int);
	void SB(unsigned int, uint16, unsigned int);
	void SW(unsigned int, uint16, unsigned int);
	void SYSCALL();

protected:
	uint32* m_ptr = nullptr;

private:
	void ResolveLabelReferences();
	void CreateLabelReference(LABEL);

	struct LABELREF
	{
		size_t address;
	};

	typedef std::map<LABEL, size_t> LabelMapType;
	typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;

	uint32* m_startPtr = nullptr;
	LabelMapType m_labels;
	LabelReferenceMapType m_labelReferences;
	unsigned int m_nextLabelId;
};
