#ifndef _JITTER_SYMBOLREF_H_
#define _JITTER_SYMBOLREF_H_

#include "Jitter_Symbol.h"

namespace Jitter
{
	class CSymbolRef
	{
	public:
		CSymbolRef(const SymbolPtr& symbol) :
		  symbol(symbol)
		{
			
		}

		SymbolPtr GetSymbol() const
		{
			return symbol.lock();
		}

		virtual std::string ToString() const
		{
			return GetSymbol()->ToString();
		}

		virtual bool Equals(CSymbolRef* symbolRef) const
		{
			if(symbolRef == NULL) return false;
			return GetSymbol()->Equals(symbolRef->GetSymbol().get());
		}

		WeakSymbolPtr symbol;
	};

	typedef std::tr1::shared_ptr<CSymbolRef> SymbolRefPtr;

	class CVersionedSymbolRef : public CSymbolRef
	{
	public:
		CVersionedSymbolRef(const SymbolPtr& symbol, int version) :
		  CSymbolRef(symbol),
		  version(version)
		{

		}

		virtual bool Equals(CSymbolRef* symbolRef) const
		{
			if(symbolRef == NULL) return false;
			if(!GetSymbol()->Equals(symbolRef->GetSymbol().get())) return false;
			CVersionedSymbolRef* versionedSymbolRef = dynamic_cast<CVersionedSymbolRef*>(symbolRef);
			if(versionedSymbolRef == NULL) return false;
			return versionedSymbolRef->version == version;
		}

		virtual std::string ToString() const
		{
			return CSymbolRef::ToString() + "{" +  boost::lexical_cast<std::string>(version) + "}";
		}

		int version;
	};

	typedef std::tr1::shared_ptr<CVersionedSymbolRef> VersionedSymbolRefPtr;

	static CSymbol* dynamic_symbolref_cast(SYM_TYPE type, const SymbolRefPtr& symbolRef)
	{
		if(!symbolRef) return NULL;
		CSymbol* result = symbolRef->GetSymbol().get();
		if(result->m_type != type) return NULL;
		return result;
	}

//	template <typename ClassType>
//	static ClassType* dynamic_symbolref_cast(const SymbolRefPtr& symbolRef)
//	{
//		if(!symbolRef) return NULL;
//		return std::tr1::dynamic_pointer_cast<ClassType>(symbolRef->GetSymbol()).get();
//	}
}

#endif
