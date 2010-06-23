#ifndef _JITTER_STATEMENT_H_
#define _JITTER_STATEMENT_H_

#include <list>
#include <functional>
#include "Jitter_SymbolRef.h"

namespace Jitter
{
	enum OPERATION
	{
		OP_NOP = 0,
		OP_MOV,
		
		OP_ADD,
		OP_SUB,
		OP_CMP,

		OP_AND,
		OP_OR,
		OP_XOR,
		OP_NOT,

		OP_SRA,
		OP_SRL,
		OP_SLL,

		OP_MUL,
		OP_MULS,
		OP_DIV,

		OP_ADD64,
		OP_EXTLOW64,
		OP_EXTHIGH64,

		OP_FP_ADD,
		OP_FP_MUL,
		OP_FP_DIV,
		OP_FP_SQRT,
		OP_FP_RCPL,
		OP_FP_NEG,

		OP_PARAM,
		OP_CALL,
		OP_RETVAL,
		OP_JMP,
		OP_CONDJMP,
		OP_LABEL,
	};

	enum CONDITION
	{
		CONDITION_NEVER = 0,
		CONDITION_EQ,
		CONDITION_NE,
		CONDITION_BL,
		CONDITION_BE,
		CONDITION_LT,
		CONDITION_AB,
		CONDITION_GT,
		CONDITION_LE,
	};

	struct STATEMENT
	{
	public:
		STATEMENT()
			: op(OP_NOP)
			, jmpBlock(-1)
			, jmpCondition(CONDITION_NEVER)
		{
			
		}

		OPERATION		op;
		SymbolRefPtr	src1;
		SymbolRefPtr	src2;
		SymbolRefPtr	dst;
		uint32			jmpBlock;
		CONDITION		jmpCondition;

		typedef std::tr1::function<void (const SymbolRefPtr&, bool)> OperandVisitor;

		void VisitOperands(const OperandVisitor& visitor)
		{
			if(dst) visitor(dst, true);
			if(src1) visitor(src1, false);
			if(src2) visitor(src2, false);
		}

		void VisitDestination(const OperandVisitor& visitor)
		{
			if(dst) visitor(dst, true);
		}

		void VisitSources(const OperandVisitor& visitor)
		{
			if(src1) visitor(src1, false);
			if(src2) visitor(src2, false);
		}
	};

	typedef std::list<STATEMENT> StatementList;
}

#endif
