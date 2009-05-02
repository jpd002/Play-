#include "ArmAssembler.h"

const uint32 g_conditionAlways = 0xE;

CArmAssembler::CArmAssembler(
                             const WriteFunctionType& WriteFunction,
                             const WriteAtFunctionType& WriteAtFunction,
                             const TellFunctionType& TellFunction
                             ) :
m_WriteFunction(WriteFunction),
m_WriteAtFunction(WriteAtFunction),
m_TellFunction(TellFunction)
{
    
}

CArmAssembler::~CArmAssembler()
{
	
}

CArmAssembler::LdrAddress CArmAssembler::MakeImmediateLdrAddress(uint32 immediate)
{
	LdrAddress result;
	memset(&result, 0, sizeof(result));
	assert((immediate & ~0xFFF) == 0);
	result.isImmediate = true;
	result.immediate = static_cast<uint16>(immediate);
	return result;
}

CArmAssembler::ImmediateAluOperand CArmAssembler::MakeImmediateAluOperand(uint8 immediate, uint8 rotateAmount)
{
	ImmediateAluOperand operand;
	operand.immediate = immediate;
	operand.rotate = rotateAmount;
	return operand;
}

void CArmAssembler::Add(REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_ADD;
	instruction.immediate = 0;
	instruction.condition = g_conditionAlways;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Add(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_ADD;
	instruction.immediate = 1;
	instruction.condition = g_conditionAlways;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Blx(REGISTER rn)
{
	uint32 opcode = 0;
	opcode = (g_conditionAlways << 28) | (0x12FFF30) | (rn);
	WriteWord(opcode);
}

void CArmAssembler::Ldr(REGISTER rd, REGISTER rbase, const LdrAddress& address)
{
	uint32 opcode = 0;
	assert(address.isImmediate);
	opcode = (g_conditionAlways << 28) | (1 << 26) | (1 << 24) | (1 << 23) | (1 << 20) | (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(rd) << 12) | (static_cast<uint32>(address.immediate));
	WriteWord(opcode);
}

void CArmAssembler::Mov(REGISTER rd, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MOV;
	instruction.immediate = 0;
	instruction.condition = g_conditionAlways;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::Str(REGISTER rd, REGISTER rbase, const LdrAddress& address)
{
	uint32 opcode = 0;
	assert(address.isImmediate);
	opcode = (g_conditionAlways << 28) | (1 << 26) | (1 << 24) | (1 << 23) | (0 << 20) | (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(rd) << 12) | (static_cast<uint32>(address.immediate));
	WriteWord(opcode);
}

void CArmAssembler::Sub(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = rn;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_SUB;
	instruction.immediate = 1;
	instruction.condition = g_conditionAlways;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CArmAssembler::WriteWord(uint32 value)
{
	m_WriteFunction(reinterpret_cast<uint8*>(&value)[0]);
	m_WriteFunction(reinterpret_cast<uint8*>(&value)[1]);
	m_WriteFunction(reinterpret_cast<uint8*>(&value)[2]);
	m_WriteFunction(reinterpret_cast<uint8*>(&value)[3]);	
}
