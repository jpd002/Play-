#include <functional>
#include <assert.h>
#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovId(CX86Assembler::rAX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 4), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow));
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 4), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel + 4), CX86Assembler::rDX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	//We can optimize here if it's equal to zero
	assert(src2->m_valueLow != 0);

	if(dst->Equals(src1))
	{
		((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), src2->m_valueLow);
	}
	else
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
		((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), src2->m_valueLow);
	}
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegTmpTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src2->m_stackLocation + m_stackLevel));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_TmpRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), CX86Assembler::rAX);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_constMatchers[] = 
{ 
	{ OP_LABEL,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::MarkLabel						},

	{ OP_NOP,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::Emit_Nop							},

//	{ OP_ADD,		MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_Add_RelRelRel				},
//	{ OP_ADD,		MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Add_RelRelCst				},
	{ OP_ADD,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegRegCst<ALUOP_ADD>	},

	{ OP_ADD64,		MATCH_RELATIVE64,	MATCH_RELATIVE64,	MATCH_RELATIVE64,	&CCodeGen_x86::Emit_Add64_RelRelRel				},

	{ OP_SUB,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegRegCst<ALUOP_SUB>	},

//	{ OP_AND,		MATCH_RELATIVE,		MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_And_RelRelCst				},
	{ OP_AND,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegRegCst<ALUOP_AND>	},

	{ OP_XOR,		MATCH_REGISTER,		MATCH_TEMPORARY,	MATCH_TEMPORARY,	&CCodeGen_x86::Emit_Alu_RegTmpTmp<ALUOP_XOR>	},
	{ OP_XOR,		MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_TmpRegReg<ALUOP_XOR>	},

	{ OP_SRL,		MATCH_TEMPORARY,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Srl_TmpRegCst				},

	{ OP_MOV,		MATCH_REGISTER,		MATCH_RELATIVE,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegRel					},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegCst					},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_TEMPORARY,	MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegTmp					},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RelCst					},
	{ OP_MOV,		MATCH_RELATIVE,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RelReg					},

	{ OP_JMP,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::Emit_Jmp							},

//	{ OP_CONDJMP,	MATCH_NIL,			MATCH_RELATIVE,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_CondJmp_RelCst				},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_CondJmp_RegCst				},

	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RegCst<false>		},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_MulTmp64RegRel<false>		},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulTmp64RegReg<false>		},

	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RegCst<true>		},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_RELATIVE,		&CCodeGen_x86::Emit_MulTmp64RegRel<true>		},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulTmp64RegReg<true>		},

	{ OP_EXTLOW64,	MATCH_REGISTER,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtLow64RegTmp64			},
	
	{ OP_EXTHIGH64,	MATCH_REGISTER,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtHigh64RegTmp64			},
	{ OP_EXTHIGH64,	MATCH_RELATIVE,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtHigh64RelTmp64			},

	{ OP_MOV,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL											},
};

CCodeGen_x86::CCodeGen_x86()
: m_registers(NULL)
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
}

CCodeGen_x86::~CCodeGen_x86()
{

}

bool CCodeGen_x86::SymbolMatches(MATCHTYPE match, const SymbolRefPtr& symbolRef)
{
	if(match == MATCH_ANY) return true;
	if(match == MATCH_NIL) { if(!symbolRef) return true; else return false; }
	CSymbol* symbol = symbolRef->GetSymbol().get();
	if(match == MATCH_RELATIVE)		{ if(symbol->m_type == SYM_RELATIVE)		return true; else return false; }
	if(match == MATCH_CONSTANT)		{ if(symbol->m_type == SYM_CONSTANT)		return true; else return false; }
	if(match == MATCH_REGISTER)		{ if(symbol->m_type == SYM_REGISTER)		return true; else return false; }
	if(match == MATCH_TEMPORARY)	{ if(symbol->m_type == SYM_TEMPORARY)		return true; else return false; }
	if(match == MATCH_RELATIVE64)	{ if(symbol->m_type == SYM_RELATIVE64)		return true; else return false; }
	if(match == MATCH_TEMPORARY64)	{ if(symbol->m_type == SYM_TEMPORARY64)		return true; else return false; }
	if(match == MATCH_CONSTANT64)	{ if(symbol->m_type == SYM_CONSTANT64)		return true; else return false; }
	return false;
}

void CCodeGen_x86::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	assert(m_registers != NULL);

	uint32 registerUsage = 0;
	for(StatementList::const_iterator statementIterator(statements.begin());
		statementIterator != statements.end(); statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);
		if(CSymbol* dst = dynamic_symbolref_cast(SYM_REGISTER, statement.dst))
		{
			registerUsage |= (1 << dst->m_valueLow);
		}
	}

	m_stackLevel = 0;
	Emit_Prolog(stackSize, registerUsage);

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

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();

	Emit_Epilog(stackSize, registerUsage);
}

void CCodeGen_x86::SetStream(Framework::CStream* stream)
{
	m_assembler.SetStream(stream);
}

CX86Assembler::LABEL CCodeGen_x86::GetLabel(uint32 blockId)
{
	CX86Assembler::LABEL result;
	LabelMapType::const_iterator labelIterator(m_labels.find(blockId));
	if(labelIterator == m_labels.end())
	{
		result = m_assembler.CreateLabel();
		m_labels[blockId] = result;
	}
	else
	{
		result = labelIterator->second;
	}
	return result;
}

void CCodeGen_x86::MarkLabel(const STATEMENT& statement)
{
	CX86Assembler::LABEL label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_x86::Emit_Nop(const STATEMENT& statement)
{
	
}

//void CCodeGen_x86::Emit_Add_RelRelRel(const STATEMENT& statement)
//{
//	CRelativeSymbol* dst = dynamic_symbolref_cast<CRelativeSymbol>(statement.dst);
//	CRelativeSymbol* src1 = dynamic_symbolref_cast<CRelativeSymbol>(statement.src1);
//	CRelativeSymbol* src2 = dynamic_symbolref_cast<CRelativeSymbol>(statement.src2);
//
//	cout << "mov eax, dword ptr[ebp + " << src1->m_valueLow << "]" << endl;
//	cout << "add eax, dword ptr[ebp + " << src2->m_valueLow << "]" << endl;
//	cout << "mov dword ptr[ebp + " << dst->m_valueLow << "], eax" << endl;
//}

//void CCodeGen_x86::Emit_Add_RelRelCst(const STATEMENT& statement)
//{
//	CRelativeSymbol* dst = dynamic_symbolref_cast<CRelativeSymbol>(statement.dst);
//	CRelativeSymbol* src1 = dynamic_symbolref_cast<CRelativeSymbol>(statement.src1);
//	CConstantSymbol* src2 = dynamic_symbolref_cast<CConstantSymbol>(statement.src2);
//
//	if(dst->Equals(src1))
//	{
//		cout << "add dword ptr[ebp + " << dst->m_valueLow << "], " << src2->m_valueLow << endl;
//	}
//	else
//	{
//		cout << "mov eax, dword ptr[ebp + " << src1->m_valueLow << "]" << endl;
//		cout << "add eax, " << src2->m_valueLow << endl;
//		cout << "mov dword ptr[ebp + " << dst->m_valueLow << "], eax" << endl;
//	}
//}

void CCodeGen_x86::Emit_Add64_RelRelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	//assert(!dst->Equals(src1));
	//assert(!dst->Equals(src2));

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 0));
	m_assembler.MovEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow + 4));

	m_assembler.AddEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 0));
	m_assembler.AdcEd(CX86Assembler::rDX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src2->m_valueLow + 4));

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 0), CX86Assembler::rAX);
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow + 4), CX86Assembler::rDX);
}

//void CCodeGen_x86::Emit_And_RelRelCst(const STATEMENT& statement)
//{
//	CRelativeSymbol* dst = dynamic_symbolref_cast<CRelativeSymbol>(statement.dst);
//	CRelativeSymbol* src1 = dynamic_symbolref_cast<CRelativeSymbol>(statement.src1);
//	CConstantSymbol* src2 = dynamic_symbolref_cast<CConstantSymbol>(statement.src2);
//
//	if(dst->Equals(src1))
//	{
//		cout << "and dword ptr[ebp + " << dst->m_valueLow << "], " << src2->m_valueLow << endl;
//	}
//	else
//	{
//		cout << "mov eax, dword ptr[ebp + " << src1->m_valueLow << "]" << endl;
//		cout << "and eax, " << src2->m_valueLow << endl;
//		cout << "mov dword ptr[ebp + " << dst->m_valueLow << "], eax" << endl;
//	}
//}

void CCodeGen_x86::Emit_Srl_TmpRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.ShrEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, dst->m_stackLocation + m_stackLevel), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Mov_RelCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovId(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), src1->m_valueLow);
}

void CCodeGen_x86::Emit_Mov_RelReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), m_registers[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Mov_RegRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
}

void CCodeGen_x86::Emit_Mov_RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	if(src1->m_valueLow == 0)
	{
		m_assembler.XorEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}
	else
	{
		m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	}
}

void CCodeGen_x86::Emit_Mov_RegTmp(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel));
}

void CCodeGen_x86::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.JmpJb(GetLabel(statement.jmpBlock));
}

void CCodeGen_x86::Emit_ExtLow64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
}

void CCodeGen_x86::Emit_ExtHigh64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
}

void CCodeGen_x86::Emit_ExtHigh64RelTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

//void CCodeGen_x86::Emit_CondJmp_RelCst(const STATEMENT& statement)
//{
//	CRelativeSymbol* src1 = dynamic_symbolref_cast<CRelativeSymbol>(statement.src1);
//	CConstantSymbol* src2 = dynamic_symbolref_cast<CConstantSymbol>(statement.src2);
//
//	cout << "cmp dword ptr[ebp + " << src1->m_valueLow << "], " << src2->m_valueLow << endl;
//
//	switch(statement.jmpCondition)
//	{
//	case CONDITION_NE:
//		cout << "jne ";
//		break;
//	default:
//		assert(0);
//		break;
//	}
//
//	cout << "$LABEL_" << statement.jmpBlock << endl;
//}

void CCodeGen_x86::Emit_CondJmp_RegCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	CX86Assembler::LABEL label(GetLabel(statement.jmpBlock));

	if((src2->m_valueLow == 0) && (statement.jmpCondition == CONDITION_NE || statement.jmpCondition == CONDITION_EQ))
	{
		m_assembler.TestEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2->m_valueLow);
	}

	switch(statement.jmpCondition)
	{
	case CONDITION_NE:
		m_assembler.JneJb(label);
		break;
	default:
		assert(0);
		break;
	}
}
