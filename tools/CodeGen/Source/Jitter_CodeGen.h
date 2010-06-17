#ifndef _JITTER_CODEGEN_H_
#define _JITTER_CODEGEN_H_

#include "Stream.h"
#include "Jitter_Statement.h"
#include <map>

namespace Jitter
{
	class CCodeGen
	{
	public:
		virtual					~CCodeGen() {};

		virtual void			SetStream(Framework::CStream*) = 0;
		virtual void			GenerateCode(const StatementList&, unsigned int) = 0;
		virtual unsigned int	GetAvailableRegisterCount() const = 0;

	protected:
		enum MATCHTYPE
		{
			MATCH_ANY,
			MATCH_NIL,

			MATCH_CONTEXT,
			MATCH_CONSTANT,
			MATCH_REGISTER,
			MATCH_RELATIVE,
			MATCH_TEMPORARY,

			MATCH_RELATIVE64,
			MATCH_TEMPORARY64,
			MATCH_CONSTANT64,

			MATCH_RELATIVE_FP_SINGLE,
		};

		typedef std::tr1::function<void (const STATEMENT&)> CodeEmitterType;

		struct MATCHER
		{
			OPERATION			op;
			MATCHTYPE			dstType;
			MATCHTYPE			src1Type;
			MATCHTYPE			src2Type;
			CodeEmitterType		emitter;
		};

		typedef std::multimap<OPERATION, MATCHER> MatcherMapType;

		bool						SymbolMatches(MATCHTYPE, const SymbolRefPtr&);

		MatcherMapType				m_matchers;
	};
}

#endif
