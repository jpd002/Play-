#ifndef _JITTER_CODEGEN_X86_64_H_
#define _JITTER_CODEGEN_X86_64_H_

#include <deque>
#include "Jitter_CodeGen_x86.h"

namespace Jitter
{
	class CCodeGen_x86_64 : public CCodeGen_x86
	{
	public:
						CCodeGen_x86_64();
		virtual			~CCodeGen_x86_64();

		unsigned int	GetAvailableRegisterCount() const;

	protected:
		virtual void						Emit_Prolog(unsigned int, uint32);
		virtual void						Emit_Epilog(unsigned int, uint32);

		//PARAM
		void								Emit_Param_Ctx(const STATEMENT&);
		void								Emit_Param_Rel(const STATEMENT&);
		void								Emit_Param_Reg(const STATEMENT&);
		void								Emit_Param_Cst(const STATEMENT&);
		void								Emit_Param_Tmp(const STATEMENT&);
		void								Emit_Param_Cst64(const STATEMENT&);

		//CALL
		void								Emit_Call(const STATEMENT&);

		//RETURNVALUE
		void								Emit_RetVal_Tmp(const STATEMENT&);
		void								Emit_RetVal_Reg(const STATEMENT&);

	private:
		typedef void (CCodeGen_x86_64::*ConstCodeEmitterType)(const STATEMENT&);

		typedef std::tr1::function<void (CX86Assembler::REGISTER)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		enum MAX_PARAMS
		{
			MAX_PARAMS = 4,
		};

		enum MAX_REGISTERS
		{
			MAX_REGISTERS = 7,
		};

		static CONSTMATCHER					g_constMatchers[];
		static CX86Assembler::REGISTER		g_registers[MAX_REGISTERS];
		static CX86Assembler::REGISTER		g_paramRegs[MAX_PARAMS];

		ParamStack							m_params;
		uint32								m_totalStackAlloc;
	};
}

#endif
