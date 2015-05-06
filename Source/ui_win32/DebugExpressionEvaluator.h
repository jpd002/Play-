#ifndef _DEBUGEXPRESSIONEVALUATOR_H_
#define _DEBUGEXPRESSIONEVALUATOR_H_

#include <vector>
#include "Types.h"
#include "../MIPS.h"

class CDebugExpressionEvaluator
{
public:
	static uint32	Evaluate(const char*, CMIPS*);

private:
	enum TOKEN_TYPE
	{
		TOKEN_TYPE_INVALID,
		TOKEN_TYPE_NUMBER,
		TOKEN_TYPE_OP,
		TOKEN_TYPE_SYMBOL,
	};

	struct TOKEN
	{
		TOKEN_TYPE type;
		std::string value;
	};

	enum OP_TYPE
	{
		OP_INVALID,
		OP_FIRST,
		OP_ADD,
		OP_SUB,
		OP_MUL,
		OP_DIV,
	};

	typedef std::vector<TOKEN> TokenArray;

	static TokenArray	Parse(const char*);
	static uint32		Evaluate(const TokenArray&, CMIPS*);
};

#endif
