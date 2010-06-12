#ifndef _JITTER_CODEGEN_ARM_H_
#define _JITTER_CODEGEN_ARM_H_

#include "Jitter_CodeGen.h"
#include "ArmAssembler.h"

namespace Jitter
{
	class CCodeGen_Arm : public CCodeGen
	{
	public:
						CCodeGen_Arm();
		virtual			~CCodeGen_Arm();

		void			GenerateCode(const StatementList&, unsigned int);
		void			SetStream(Framework::CStream*);
		unsigned int	GetAvailableRegisterCount() const;

	private:
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

		typedef void (CCodeGen_Arm::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		static CONSTMATCHER			g_constMatchers[];

		bool						SymbolMatches(MATCHTYPE, const SymbolRefPtr&);
		CArmAssembler::LABEL		GetLabel(uint32);

		MatcherMapType				m_matchers;
		CArmAssembler				m_assembler;
	};
};

#endif
