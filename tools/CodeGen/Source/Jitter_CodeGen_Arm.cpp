#include "Jitter_CodeGen_Arm.h"

using namespace Jitter;

CCodeGen_Arm::CCodeGen_Arm()
{

}

CCodeGen_Arm::~CCodeGen_Arm()
{

}

unsigned int CCodeGen_Arm::GetAvailableRegisterCount() const
{
	return 3;
}

void CCodeGen_Arm::SetStream(Framework::CStream* stream)
{
	m_assembler.SetStream(stream);
}

void CCodeGen_Arm::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	//for(StatementList::const_iterator statementIterator(statements.begin());
	//	statementIterator != statements.end(); statementIterator++)
	//{
	//	const STATEMENT& statement(*statementIterator);

	//	bool found = false;
	//	MatcherMapType::const_iterator begin = m_matchers.lower_bound(statement.op);
	//	MatcherMapType::const_iterator end = m_matchers.upper_bound(statement.op);

	//	for(MatcherMapType::const_iterator matchIterator(begin); matchIterator != end; matchIterator++)
	//	{
	//		const MATCHER& matcher(matchIterator->second);
	//		if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
	//		if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
	//		if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
	//		matcher.emitter(statement);
	//		found = true;
	//		break;
	//	}
	//	assert(found);
	//}

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();
}
