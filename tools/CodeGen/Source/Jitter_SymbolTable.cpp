#include <assert.h>
#include "Jitter_SymbolTable.h"

using namespace Jitter;

CSymbolTable::CSymbolTable()
{

}

CSymbolTable::~CSymbolTable()
{

}

CSymbolTable::SymbolIterator CSymbolTable::GetSymbolsBegin()
{
	return m_symbols.begin();
}

CSymbolTable::SymbolIterator CSymbolTable::GetSymbolsEnd()
{
	return m_symbols.end();
}

CSymbolTable::SymbolIterator CSymbolTable::RemoveSymbol(const SymbolIterator& symbolIterator)
{
	return m_symbols.erase(symbolIterator);
}

SymbolPtr CSymbolTable::MakeSymbol(const SymbolPtr& srcSymbol)
{
	SymbolSet::const_iterator symbolIterator(m_symbols.find(srcSymbol));
	if(symbolIterator != m_symbols.end())
	{
		return *symbolIterator;
	}
	SymbolPtr result = SymbolPtr(new CSymbol(*srcSymbol));
	m_symbols.insert(result);
	return result;
}

SymbolPtr CSymbolTable::MakeSymbol(SYM_TYPE type, uint32 valueLow, uint32 valueHigh)
{
	CSymbol symbol(type, valueLow, valueHigh);
	return MakeSymbol(SymbolPtr(&symbol, SymbolNullDeleter()));
}
