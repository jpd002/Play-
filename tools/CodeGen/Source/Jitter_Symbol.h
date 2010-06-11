#ifndef _JITTER_SYMBOL_H_
#define _JITTER_SYMBOL_H_

#include "Types.h"
#include <memory>
#include <string>
#include <boost/lexical_cast.hpp>
#include <assert.h>

namespace Jitter
{
	enum SYM_TYPE
	{
		SYM_CONTEXT,
		SYM_CONSTANT,
		SYM_RELATIVE,
		SYM_TEMPORARY,
		SYM_REGISTER,
		SYM_RELATIVE64,
		SYM_TEMPORARY64,
		SYM_CONSTANT64,

		SYM_FP_REL_SINGLE,
		SYM_FP_TMP_SINGLE,
	};

	class CSymbol
	{
	public:
		CSymbol(SYM_TYPE type, uint32 valueLow, uint32 valueHigh)
			: m_type(type)
			, m_valueLow(valueLow)
			, m_valueHigh(valueHigh)
		{
			m_rangeBegin = -1;
			m_rangeEnd = -1;
			m_firstUse = -1;
			m_firstDef = -1;
			m_register = -1;
			m_stackLocation = -1;
			m_useCount = 0;
			m_aliased = false;
		}

		virtual ~CSymbol()
		{
			
		}

		std::string ToString() const
		{
			switch(m_type)
			{
			case SYM_CONTEXT:
				return "CTX";
				break;
			case SYM_CONSTANT:
				return boost::lexical_cast<std::string>(m_valueLow);
				break;
			case SYM_RELATIVE:
				return "REL[" + boost::lexical_cast<std::string>(m_valueLow) + "]";
				break;
			case SYM_TEMPORARY:
				return "TMP[" + boost::lexical_cast<std::string>(m_valueLow) + "]";
				break;
			case SYM_CONSTANT64:
				return "CST64[" + boost::lexical_cast<std::string>(m_valueLow) + ", " + boost::lexical_cast<std::string>(m_valueHigh) + "]";
				break;
			case SYM_TEMPORARY64:
				return "TMP64[" + boost::lexical_cast<std::string>(m_valueLow) + "]";
				break;
			case SYM_RELATIVE64:
				return "REL64[" + boost::lexical_cast<std::string>(m_valueLow) + "]";
				break;
			case SYM_REGISTER:
				return "REG[" + boost::lexical_cast<std::string>(m_valueLow) + "]";
				break;
			case SYM_FP_REL_SINGLE:
				return "REL(FP_S)[" + boost::lexical_cast<std::string>(m_valueLow) + "]";
				break;
			case SYM_FP_TMP_SINGLE:
				return "TMP(FP_S)[" + boost::lexical_cast<std::string>(m_valueLow) + "]";
				break;
			default:
				return "";
				break;
			}
		}

		int GetSize() const
		{
			switch(m_type)
			{
			case SYM_RELATIVE:
			case SYM_REGISTER:
			case SYM_CONSTANT:
			case SYM_TEMPORARY:
				return 4;
				break;
			case SYM_RELATIVE64:
			case SYM_TEMPORARY64:
			case SYM_CONSTANT64:
				return 8;
				break;
			case SYM_FP_REL_SINGLE:
			case SYM_FP_TMP_SINGLE:
				return 4;
				break;
			default:
				assert(0);
				return 4;
				break;
			}
		}

		bool IsRelative() const
		{
			return (m_type == SYM_RELATIVE) || (m_type == SYM_RELATIVE64) || (m_type == SYM_FP_REL_SINGLE);
		}

		bool Equals(CSymbol* symbol) const
		{
			return 
				(symbol) &&
				(symbol->m_type == m_type) &&
				(symbol->m_valueLow == m_valueLow) &&
				(symbol->m_valueHigh == m_valueHigh);
		}

		bool Aliases(CSymbol* symbol) const
		{
			if(!IsRelative()) return false;
			if(!symbol->IsRelative()) return false;
			uint32 base1 = m_valueLow;
			uint32 base2 = symbol->m_valueLow;
			uint32 end1 = base1 + GetSize();
			uint32 end2 = base2 + symbol->GetSize();
			if(abs(static_cast<int32>(base2 - base1)) < GetSize()) return true;
			if(abs(static_cast<int32>(base2 - base1)) < symbol->GetSize()) return true;
			return false;
		}

		SYM_TYPE				m_type;
		uint32					m_valueLow;
		uint32					m_valueHigh;

		unsigned int			m_useCount;
		unsigned int			m_firstUse;
		unsigned int			m_firstDef;
		unsigned int			m_rangeBegin;
		unsigned int			m_rangeEnd;
		unsigned int			m_register;
		unsigned int			m_stackLocation;
		bool					m_aliased;
	};

	typedef std::tr1::shared_ptr<CSymbol> SymbolPtr;
	typedef std::tr1::weak_ptr<CSymbol> WeakSymbolPtr;
}

#endif
