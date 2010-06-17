#ifndef _ARMASSEMBLER_H_
#define _ARMASSEMBLER_H_

#include <functional>
#include <map>
#include "Stream.h"
#include "Types.h"

class CArmAssembler
{
public:
	enum REGISTER
	{
		r0 = 0,
		r1 = 1,
		r2 = 2,
		r3 = 3,
		r4 = 4,
		r5 = 5,
		r6 = 6,
		r7 = 7,
		r8 = 8,
		r9 = 9,
		r10 = 10,
		r11 = 11,
		r12 = 12,
		r13 = 13,
		r14 = 14,
		r15 = 15,
		
		rIP = 12,		
		rSP = 13,
		rLR = 14,		
		rPC = 15,
	};
	
	enum ALU_OPCODE
	{
		ALU_OPCODE_AND = 0x00,
		ALU_OPCODE_EOR = 0x01,
		ALU_OPCODE_SUB = 0x02,
		ALU_OPCODE_RSB = 0x03,
		ALU_OPCODE_ADD = 0x04,
		ALU_OPCODE_ADC = 0x05,
		ALU_OPCODE_SBC = 0x06,
		ALU_OPCODE_RSC = 0x07,
		ALU_OPCODE_TST = 0x08,
		ALU_OPCODE_TEQ = 0x09,
		ALU_OPCODE_CMP = 0x0A,
		ALU_OPCODE_CMN = 0x0B,
		ALU_OPCODE_ORR = 0x0C,
		ALU_OPCODE_MOV = 0x0D,
		ALU_OPCODE_BIC = 0x0E,
		ALU_OPCODE_MVN = 0x0F,
	};
	
	enum CONDITION
	{
		CONDITION_EQ = 0x00,
		CONDITION_NE = 0x01,
		CONDITION_CS = 0x02,
		CONDITION_CC = 0x03,
		CONDITION_MI = 0x04,
		CONDITION_PL = 0x05,
		CONDITION_VS = 0x06,
		CONDITION_VC = 0x07,
		CONDITION_HI = 0x08,
		CONDITION_LS = 0x09,
		CONDITION_GE = 0x0A,
		CONDITION_LT = 0x0B,
		CONDITION_GT = 0x0C,
		CONDITION_LE = 0x0D,
		CONDITION_AL = 0x0E,
		CONDITION_NV = 0x0F
	};
	
	enum SHIFT
	{
		SHIFT_LSL = 0x00,
		SHIFT_LSR = 0x01,
		SHIFT_ASR = 0x02,
		SHIFT_ROR = 0x03,
	};
	
	struct InstructionAlu
	{
		InstructionAlu()
		{
			memset(this, 0, sizeof(InstructionAlu));
		}
		
		unsigned int		operand		: 12;
		unsigned int		rd			: 4;
		unsigned int		rn			: 4;
		unsigned int		setFlags	: 1;
		unsigned int		opcode		: 4;
		unsigned int		immediate	: 1;
		unsigned int		reserved	: 2;
		unsigned int		condition	: 4;
	};
	
	struct ImmediateAluOperand
	{
		unsigned int	immediate	: 8;
		unsigned int	rotate		: 4;
	};

	struct RegisterAluOperand
	{
		unsigned int	rm			: 4;
		unsigned int	shift		: 8;
	};
	
	struct AluLdrShift
	{
		unsigned int	typeBit		: 1;
		unsigned int	type		: 2;
		unsigned int	amount		: 5;
	};
	
	struct ShiftRmAddress
	{
		union
		{
			struct
			{
				unsigned int	rm			: 4;
				unsigned int	isConstant	: 1;
				unsigned int	shiftType	: 2;
				unsigned int	constant	: 5;
			} immediateShift;
			struct
			{
				unsigned int	rm			: 4;
				unsigned int	isConstant	: 1;
				unsigned int	shiftType	: 2;
				unsigned int	zero		: 1;
				unsigned int	rs			: 4;
			} registerShift;
		};
	};

	struct LdrAddress
	{
		union
		{
			uint16			immediate;
			ShiftRmAddress	shiftRm;
		};
		bool isImmediate;
	};
	
	typedef unsigned int LABEL;
	
	
											CArmAssembler();
	virtual									~CArmAssembler();

	void									SetStream(Framework::CStream*);

	LABEL									CreateLabel();
	void									ClearLabels();
	void									MarkLabel(LABEL);
	void									ResolveLabelReferences();
	
	void									Add(REGISTER, REGISTER, REGISTER);
	void									Add(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									And(REGISTER, REGISTER, REGISTER);
	void									BCc(CONDITION, LABEL);
	void									Blx(REGISTER);
	void									Cmp(REGISTER, REGISTER);
	void									Eor(REGISTER, REGISTER, REGISTER);
	void									Ldr(REGISTER, REGISTER, const LdrAddress&);
	void									Mov(REGISTER, REGISTER);
	void									Mov(REGISTER, const RegisterAluOperand&);
	void									Mov(REGISTER, const ImmediateAluOperand&);
	void									MovCc(CONDITION, REGISTER, const ImmediateAluOperand&);
	void									Mvn(REGISTER, REGISTER);
	void									Or(REGISTER, REGISTER, REGISTER);
	void									Smull(REGISTER, REGISTER, REGISTER, REGISTER);
	void									Str(REGISTER, REGISTER, const LdrAddress&);
	void									Sub(REGISTER, REGISTER, REGISTER);
	void									Sub(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Teq(REGISTER, const ImmediateAluOperand&);
	
	static LdrAddress						MakeImmediateLdrAddress(uint32);
	static ImmediateAluOperand				MakeImmediateAluOperand(uint8, uint8);
	static RegisterAluOperand				MakeRegisterAluOperand(CArmAssembler::REGISTER, const AluLdrShift&);
	
	static AluLdrShift						MakeConstantShift(CArmAssembler::SHIFT, uint8);
	static AluLdrShift						MakeVariableShift(CArmAssembler::SHIFT, CArmAssembler::REGISTER);
	
private:
	typedef size_t LABELREF;
	
    typedef std::map<LABEL, size_t> LabelMapType;
    typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;
	
	void									CreateLabelReference(LABEL);
	void									WriteWord(uint32);
	
	unsigned int							m_nextLabelId;
	LabelMapType							m_labels;
	LabelReferenceMapType					m_labelReferences;
	
	Framework::CStream*						m_stream;
};

#endif
