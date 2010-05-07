#include "Jitter_CodeGen_x86_64.h"

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_64::g_registers[7] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
	CX86Assembler::r12,
	CX86Assembler::r13,
	CX86Assembler::r14,
	CX86Assembler::r15,
};

CX86Assembler::REGISTER CCodeGen_x86_64::g_paramRegs[MAX_PARAMS] =
{
	CX86Assembler::rCX,
	CX86Assembler::rDX,
	CX86Assembler::r8,
	CX86Assembler::r9,
};

CCodeGen_x86_64::CONSTMATCHER CCodeGen_x86_64::g_constMatchers[] = 
{ 
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Cst64		},

	{ OP_CALL,		MATCH_NIL,			MATCH_CONSTANT64,	MATCH_CONSTANT,		&CCodeGen_x86_64::Emit_Call				},

	{ OP_RETVAL,	MATCH_TEMPORARY,	MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_64::Emit_RetVal_Tmp		},
};

CCodeGen_x86_64::CCodeGen_x86_64()
{
	CCodeGen_x86::m_registers = g_registers;

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
}

CCodeGen_x86_64::~CCodeGen_x86_64()
{

}

unsigned int CCodeGen_x86_64::GetAvailableRegisterCount() const
{
	return 7;
}

void CCodeGen_x86_64::Emit_Prolog(unsigned int stackSize)
{
	m_currentParam = 0;
}

void CCodeGen_x86_64::Emit_Epilog(unsigned int stackSize)
{

}

void CCodeGen_x86_64::Emit_Param_Cst64(const STATEMENT& statement)
{
	assert(m_currentParam < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();
//	m_assembler.MovIq(g_paramRegs[m_currentParam], 
//		(static_cast<uint64>(src1->m_valueHigh) << 32) | static_cast<uint64>(src1->m_valueLow));
	
	m_currentParam++;
}

void CCodeGen_x86_64::Emit_Call(const STATEMENT& statement)
{

}

void CCodeGen_x86_64::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation), CX86Assembler::rAX);
}
