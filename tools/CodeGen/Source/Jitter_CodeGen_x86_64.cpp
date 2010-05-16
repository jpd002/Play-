#include "Jitter_CodeGen_x86_64.h"

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_64::g_registers[MAX_REGISTERS] =
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
	{ OP_PARAM,		MATCH_NIL,			MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Tmp		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Cst64		},

	{ OP_CALL,		MATCH_NIL,			MATCH_CONSTANT64,	MATCH_CONSTANT,		&CCodeGen_x86_64::Emit_Call				},

	{ OP_RETVAL,	MATCH_TEMPORARY,	MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_64::Emit_RetVal_Tmp		},
};

static uint64 CombineConstant64(uint32 cstLow, uint32 cstHigh)
{
	return (static_cast<uint64>(cstHigh) << 32) | static_cast<uint64>(cstLow);
}

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
	return MAX_REGISTERS;
}

void CCodeGen_x86_64::Emit_Prolog(unsigned int stackSize, uint32 registerUsage)
{
	m_params.clear();

	m_assembler.Push(CX86Assembler::rBP);
	m_assembler.MovEq(CX86Assembler::rBP, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));

	uint32 savedCount = 0;
	for(unsigned int i = 0; i < MAX_REGISTERS; i++)
	{
		if(registerUsage & (1 << i))
		{
			m_assembler.Push(m_registers[i]);
			savedCount += 8;
		}
	}

	savedCount = 0x10 - (savedCount & 0xF);

	m_totalStackAlloc = savedCount + ((stackSize + 0xF) & ~0xF);
	m_totalStackAlloc += 0x20;

	m_stackLevel = 0x20;

	m_assembler.SubIq(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc);
}

void CCodeGen_x86_64::Emit_Epilog(unsigned int stackSize, uint32 registerUsage)
{
	m_assembler.AddIq(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), m_totalStackAlloc);

	for(int i = MAX_REGISTERS - 1; i >= 0; i--)
	{
		if(registerUsage & (1 << i))
		{
			m_assembler.Pop(m_registers[i]);
		}
	}

	m_assembler.Pop(CX86Assembler::rBP);
	m_assembler.Ret();
}

void CCodeGen_x86_64::Emit_Param_Tmp(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_params.push_back(std::tr1::bind(
		&CX86Assembler::MovEd, &m_assembler, std::tr1::placeholders::_1, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel)));
}

void CCodeGen_x86_64::Emit_Param_Cst64(const STATEMENT& statement)
{
	assert(m_params.size() < MAX_PARAMS);

	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_params.push_back(std::tr1::bind(
		&CX86Assembler::MovIq, &m_assembler, std::tr1::placeholders::_1, CombineConstant64(src1->m_valueLow, src1->m_valueHigh)));
}

void CCodeGen_x86_64::Emit_Call(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	unsigned int paramCount = src2->m_valueLow;

	for(unsigned int i = 0; i < paramCount; i++)
	{
		ParamEmitterFunction emitter(m_params.back());
		m_params.pop_back();
		emitter(g_paramRegs[i]);
	}

	m_assembler.MovIq(CX86Assembler::rAX, CombineConstant64(src1->m_valueLow, src1->m_valueHigh));
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86_64::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), CX86Assembler::rAX);
}
