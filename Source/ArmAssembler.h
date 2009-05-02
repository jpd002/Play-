#ifndef _ARMASSEMBLER_H_
#define _ARMASSEMBLER_H_

#include <functional>
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
	
    typedef std::tr1::function<void (uint8)>                WriteFunctionType;
    typedef std::tr1::function<void (unsigned int, uint8)>  WriteAtFunctionType;
    typedef std::tr1::function<size_t ()>                   TellFunctionType;
	
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
	
	
											CArmAssembler(
														  const WriteFunctionType&, 
														  const WriteAtFunctionType&,
														  const TellFunctionType&);
	
	virtual									~CArmAssembler();
	
	void									Add(REGISTER, REGISTER, REGISTER);
	void									Add(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Blx(REGISTER);
	void									Ldr(REGISTER, REGISTER, const LdrAddress&);
	void									Mov(REGISTER, REGISTER);
	void									Str(REGISTER, REGISTER, const LdrAddress&);
	void									Sub(REGISTER, REGISTER, const ImmediateAluOperand&);
	
	static LdrAddress						MakeImmediateLdrAddress(uint32);
	static ImmediateAluOperand				MakeImmediateAluOperand(uint8, uint8);
	
private:
	void									WriteWord(uint32);
	
    WriteFunctionType                       m_WriteFunction;
    WriteAtFunctionType                     m_WriteAtFunction;
    TellFunctionType                        m_TellFunction;
};

#endif
