#include "Jitter_CodeGen.h"

using namespace Jitter;

bool CCodeGen::SymbolMatches(MATCHTYPE match, const SymbolRefPtr& symbolRef)
{
	if(match == MATCH_ANY) return true;
	if(match == MATCH_NIL) { if(!symbolRef) return true; else return false; }
	CSymbol* symbol = symbolRef->GetSymbol().get();
	switch(match)
	{
	case MATCH_RELATIVE:
		return symbol->m_type == SYM_RELATIVE;
	case MATCH_CONSTANT:
		return symbol->m_type == SYM_CONSTANT;
	case MATCH_REGISTER:
		return symbol->m_type == SYM_REGISTER;
	case MATCH_TEMPORARY:
		return symbol->m_type == SYM_TEMPORARY;
	case MATCH_RELATIVE64:
		return symbol->m_type == SYM_RELATIVE64;
	case MATCH_TEMPORARY64:
		return symbol->m_type == SYM_TEMPORARY64;
	case MATCH_CONSTANT64:
		return symbol->m_type == SYM_CONSTANT64;
	case MATCH_RELATIVE_FP_SINGLE:
		return symbol->m_type == SYM_FP_REL_SINGLE;
	case MATCH_CONTEXT:
		return symbol->m_type == SYM_CONTEXT;
	}
	return false;
}
