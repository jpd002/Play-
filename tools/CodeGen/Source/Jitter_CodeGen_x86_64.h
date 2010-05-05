#ifndef _JITTER_CODEGEN_X86_64_H_
#define _JITTER_CODEGEN_X86_64_H_

#include "Jitter_CodeGen_x86.h"

namespace Jitter
{
	class CCodeGen_x86_64 : public CCodeGen_x86
	{
	public:
						CCodeGen_x86_64();
		virtual			~CCodeGen_x86_64();

		unsigned int	GetAvailableRegisterCount() const;

	private:
		typedef void (CCodeGen_x86_64::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		static CONSTMATCHER					g_constMatchers[];
		static CX86Assembler::REGISTER		g_registers[];
	};
}

#endif
