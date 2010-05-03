#ifndef _JITTER_SYMBOLTABLE_H_
#define _JITTER_SYMBOLTABLE_H_

#include <unordered_set>
#include "Jitter_Symbol.h"

namespace Jitter
{
	class CSymbolTable
	{
	public:
		struct SymbolComparator
		{
			bool operator()(const SymbolPtr& sym1, const SymbolPtr& sym2) const
			{
				return sym1->Equals(sym2.get());
			}
		};

		struct SymbolHasher
		{
			size_t operator()(const SymbolPtr& symbol) const
			{
				return symbol->m_type ^ symbol->m_valueLow ^ symbol->m_valueHigh;
			}
		};

		struct SymbolNullDeleter
		{
			void operator()(void const *) const
			{

			}
		};

		typedef std::tr1::unordered_set<SymbolPtr, SymbolHasher, SymbolComparator> SymbolSet;
		typedef SymbolSet::iterator SymbolIterator;

								CSymbolTable();
		virtual					~CSymbolTable();

		SymbolPtr				MakeSymbol(const SymbolPtr&);
		SymbolPtr				MakeSymbol(SYM_TYPE, uint32, uint32 = 0);
		SymbolIterator			RemoveSymbol(const SymbolIterator&);

		SymbolIterator			GetSymbolsBegin();
		SymbolIterator			GetSymbolsEnd();

	private:
		SymbolSet				m_symbols;
	};
}

#endif
