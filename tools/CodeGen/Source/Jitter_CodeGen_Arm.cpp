#include "Jitter_CodeGen_Arm.h"

using namespace Jitter;

CArmAssembler::REGISTER CCodeGen_Arm::g_baseRegister = CArmAssembler::r11;

CArmAssembler::REGISTER CCodeGen_Arm::g_registers[MAX_REGISTERS] =
{
	CArmAssembler::r4,
	CArmAssembler::r5,
	CArmAssembler::r6,
	CArmAssembler::r7,
	CArmAssembler::r8,
	CArmAssembler::r10,
};

CCodeGen_Arm::CONSTMATCHER CCodeGen_Arm::g_constMatchers[] = 
{ 
	{ OP_LABEL,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_Arm::MarkLabel							},

	{ OP_MOV,		MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_RegCst						},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_Arm::Emit_Mov_RelReg						},

	{ OP_MOV,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL												},
};

CCodeGen_Arm::CCodeGen_Arm()
: m_stream(NULL)
, m_literalPool(NULL)
{
	for(CONSTMATCHER* constMatcher = g_constMatchers; constMatcher->emitter != NULL; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::tr1::bind(constMatcher->emitter, this, std::tr1::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}

	m_literalPool = new uint32[LITERAL_POOL_SIZE];
}

CCodeGen_Arm::~CCodeGen_Arm()
{
	if(m_literalPool)
	{
		delete [] m_literalPool;
		m_literalPool = NULL;
	}
}

unsigned int CCodeGen_Arm::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

void CCodeGen_Arm::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
	m_assembler.SetStream(stream);
}

void CCodeGen_Arm::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	m_lastLiteralPtr = 0;

	//Save return address
	m_assembler.Sub(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(0x04, 0));
	m_assembler.Str(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0x00));

	for(StatementList::const_iterator statementIterator(statements.begin());
		statementIterator != statements.end(); statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);

		bool found = false;
		MatcherMapType::const_iterator begin = m_matchers.lower_bound(statement.op);
		MatcherMapType::const_iterator end = m_matchers.upper_bound(statement.op);

		for(MatcherMapType::const_iterator matchIterator(begin); matchIterator != end; matchIterator++)
		{
			const MATCHER& matcher(matchIterator->second);
			if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
			if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
			if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
			matcher.emitter(statement);
			found = true;
			break;
		}
		assert(found);
	}

	m_assembler.Ldr(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0x00));
	m_assembler.Add(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(0x04, 0));
	m_assembler.Blx(CArmAssembler::rLR);

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();

	DumpLiteralPool();
}

uint32 CCodeGen_Arm::RotateRight(uint32 value)
{
	uint32 carry = value & 1;
	value >>= 1;
	value |= carry << 31;
	return value;
}

uint32 CCodeGen_Arm::RotateLeft(uint32 value)
{
	uint32 carry = value >> 31;
	value <<= 1;
	value |= carry;
	return value;
}

void CCodeGen_Arm::LoadConstantInRegister(CArmAssembler::REGISTER registerId, uint32 constant)
{
	uint32 shadowConstant = constant;
	int shiftAmount = -1;
	for(unsigned int i = 0; i < 16; i++)
	{
		if((shadowConstant & 0xFF) == shadowConstant)
		{
			shiftAmount = i;
			break;
		}
		shadowConstant = RotateLeft(shadowConstant);
		shadowConstant = RotateLeft(shadowConstant);
	}
	
	if(shiftAmount != -1)
	{
		m_assembler.Mov(registerId, CArmAssembler::MakeImmediateAluOperand(static_cast<uint8>(shadowConstant), shiftAmount));
	}
	else
	{
		//Search for an existing literal
		unsigned int literalPtr = -1;
		for(unsigned int i = 0; i < m_lastLiteralPtr; i++)
		{
			if(m_literalPool[i] == constant) 
			{
				literalPtr = i;
				break;
			}
		}
		if(literalPtr == -1)
		{
			assert(m_lastLiteralPtr != LITERAL_POOL_SIZE);
			literalPtr = m_lastLiteralPtr++;
			m_literalPool[literalPtr] = constant;
		}
		
		LITERAL_POOL_REF reference;
		reference.poolPtr = literalPtr;
		reference.dstRegister = registerId;
		reference.offset = static_cast<unsigned int>(m_stream->Tell());
		m_literalPoolRefs.push_back(reference);
		
		//Write a blank instruction
		m_stream->Write32(0);
	}
}

void CCodeGen_Arm::DumpLiteralPool()
{
	if(m_lastLiteralPtr)
	{
		uint32 literalPoolPos = static_cast<uint32>(m_stream->Tell());
		m_stream->Write(m_literalPool, sizeof(uint32) * m_lastLiteralPtr);
		
		for(LiteralPoolRefList::const_iterator referenceIterator(m_literalPoolRefs.begin());
			referenceIterator != m_literalPoolRefs.end(); referenceIterator++)
		{
			const LITERAL_POOL_REF& reference(*referenceIterator);
			m_stream->Seek(reference.offset, Framework::STREAM_SEEK_SET);
			uint32 literalOffset = (reference.poolPtr * 4 + literalPoolPos) - reference.offset - 8;
			m_assembler.Ldr(reference.dstRegister, CArmAssembler::rPC, CArmAssembler::MakeImmediateLdrAddress(literalOffset));
		}
	}
	
	m_literalPoolRefs.clear();
	m_lastLiteralPtr = 0;
}

void CCodeGen_Arm::MarkLabel(const STATEMENT& statement)
{
//	CArmAssembler::LABEL label = GetLabel(statement.jmpBlock);
//	m_assembler.MarkLabel(label);
}

void CCodeGen_Arm::Emit_Mov_RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	LoadConstantInRegister(g_registers[dst->m_valueLow], src1->m_valueLow);
}

void CCodeGen_Arm::Emit_Mov_RelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.Str(g_registers[src1->m_valueLow], g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(dst->m_valueLow));
}
