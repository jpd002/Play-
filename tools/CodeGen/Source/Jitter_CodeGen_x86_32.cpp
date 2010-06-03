#include "Jitter_CodeGen_x86_32.h"

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_32::g_registers[3] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
};

CCodeGen_x86_32::CONSTMATCHER CCodeGen_x86_32::g_constMatchers[] = 
{ 
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONTEXT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Ctx		},
//	{ OP_PARAM,		MATCH_NIL,			MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Rel		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Cst		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Reg		},
	{ OP_PARAM,		MATCH_NIL,			MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86_32::Emit_Param_Tmp		},

	{ OP_CALL,		MATCH_NIL,			MATCH_CONSTANT,		MATCH_CONSTANT,		&CCodeGen_x86_32::Emit_Call				},

	{ OP_RETVAL,	MATCH_TEMPORARY,	MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Tmp		},
	{ OP_RETVAL,	MATCH_REGISTER,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86_32::Emit_RetVal_Reg		},
};

CCodeGen_x86_32::CCodeGen_x86_32()
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

CCodeGen_x86_32::~CCodeGen_x86_32()
{

}

void CCodeGen_x86_32::Emit_Prolog(unsigned int stackSize, uint32 registerUsage)
{
	//Allocate stack space
	if(stackSize != 0)
	{
		m_assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), stackSize);
	}
}

void CCodeGen_x86_32::Emit_Epilog(unsigned int stackSize, uint32 registerUsage)
{
	if(stackSize != 0)
	{
		m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), stackSize);
	}
	m_assembler.Ret();
}

unsigned int CCodeGen_x86_32::GetAvailableRegisterCount() const
{
	//We have ebx, esi and edi
	return 3;
}

//void CCodeGen_x86::Emit_Param_Rel(const STATEMENT& statement)
//{
//	CRelativeSymbol* src1 = dynamic_symbolref_cast<CRelativeSymbol>(statement.src1);
//
//	cout << "push dword ptr[ebp + " << src1->m_valueLow << "]" << endl;
//}

void CCodeGen_x86_32::Emit_Param_Ctx(const STATEMENT& statement)
{
	m_assembler.Push(CX86Assembler::rBP);
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Param_Cst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_assembler.PushId(src1->m_valueLow);
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Param_Reg(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_assembler.Push(m_registers[src1->m_valueLow]);
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Param_Tmp(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	m_assembler.PushEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	m_stackLevel += 4;
}

void CCodeGen_x86_32::Emit_Call(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.CallEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), src2->m_valueLow * 4);

	m_stackLevel = 0;
}

void CCodeGen_x86_32::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation), CX86Assembler::rAX);
}

void CCodeGen_x86_32::Emit_RetVal_Reg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();

	m_assembler.MovGd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), CX86Assembler::rAX);
}
