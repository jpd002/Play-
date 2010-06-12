#ifndef _JITTER_CODEGEN_X86_H_
#define _JITTER_CODEGEN_X86_H_

#include "Jitter_CodeGen.h"
#include "X86Assembler.h"

namespace Jitter
{
	class CCodeGen_x86 : public CCodeGen
	{
	public:
						CCodeGen_x86();
		virtual			~CCodeGen_x86();

		void			GenerateCode(const StatementList&, unsigned int);
		void			SetStream(Framework::CStream*);

	protected:
		typedef std::map<uint32, CX86Assembler::LABEL> LabelMapType;

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

		//ALUOP ----------------------------------------------------------
		struct ALUOP_BASE
		{
			typedef void (CX86Assembler::*OpIdType)(const CX86Assembler::CAddress&, uint32);
			typedef void (CX86Assembler::*OpEdType)(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
		};

		struct ALUOP_ADD : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::AddId; }
			static OpEdType OpEd() { return &CX86Assembler::AddEd; }
		};

		struct ALUOP_AND : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::AndId; }
			static OpEdType OpEd() { return &CX86Assembler::AndEd; }
		};

		struct ALUOP_SUB : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::SubId; }
			static OpEdType OpEd() { return &CX86Assembler::SubEd; }
		};

		struct ALUOP_OR : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::OrId; }
			static OpEdType OpEd() { return &CX86Assembler::OrEd; }
		};

		struct ALUOP_XOR : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::XorId; }
			static OpEdType OpEd() { return &CX86Assembler::XorEd; }
		};

		//SHIFTOP -----------------------------------------------------------
		struct SHIFTOP_BASE
		{
			typedef void (CX86Assembler::*OpCstType)(const CX86Assembler::CAddress&, uint8);
			typedef void (CX86Assembler::*OpVarType)(const CX86Assembler::CAddress&);
		};

		struct SHIFTOP_SRL : public SHIFTOP_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::ShrEd; }
			static OpVarType OpVar() { return &CX86Assembler::ShrEd; }
		};

		struct SHIFTOP_SRA : public SHIFTOP_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::SarEd; }
			static OpVarType OpVar() { return &CX86Assembler::SarEd; }
		};

		struct SHIFTOP_SLL : public SHIFTOP_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::ShlEd; }
			static OpVarType OpVar() { return &CX86Assembler::ShlEd; }
		};

		//FPUOP -----------------------------------------------------------
		struct FPUOP_BASE
		{
			typedef void (CX86Assembler::*OpEdType)(CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
		};

		struct FPUOP_ADD : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::AddssEd; }
		};

		struct FPUOP_DIV : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::DivssEd; }
		};
		
		struct FPUOP_RCPL : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::RcpssEd; }
		};

		virtual void				Emit_Prolog(unsigned int, uint32) = 0;
		virtual void				Emit_Epilog(unsigned int, uint32) = 0;

		bool						SymbolMatches(MATCHTYPE, const SymbolRefPtr&);
		CX86Assembler::LABEL		GetLabel(uint32);

		//LABEL
		void						MarkLabel(const STATEMENT&);

		//NOP
		void						Emit_Nop(const STATEMENT&);

		//ALU
		template <typename> void	Emit_Alu_RegRegCst(const STATEMENT&);
		template <typename> void	Emit_Alu_RegRelReg(const STATEMENT&);
		template <typename> void	Emit_Alu_RegRegReg(const STATEMENT&);
		template <typename> void	Emit_Alu_RegTmpTmp(const STATEMENT&);
		template <typename> void	Emit_Alu_RegTmpCst(const STATEMENT&);
		template <typename> void	Emit_Alu_RegCstReg(const STATEMENT&);
		template <typename> void	Emit_Alu_TmpRegReg(const STATEMENT&);
		template <typename> void	Emit_Alu_TmpTmpCst(const STATEMENT&);

		//SHIFT
		template <typename> void	Emit_Shift_TmpRegCst(const STATEMENT&);
		template <typename> void	Emit_Shift_RegRegCst(const STATEMENT&);
		template <typename> void	Emit_Shift_RegRelCst(const STATEMENT&);

		//NOT
		void						Emit_Not_RegReg(const STATEMENT&);
		void						Emit_Not_RegRel(const STATEMENT&);

		//ADD64
		void						Emit_Add64_RelRelRel(const STATEMENT&);

		//MUL/MULS
		template<bool> void			Emit_MulTmp64RegRel(const STATEMENT&);
		template<bool> void			Emit_MulTmp64RegCst(const STATEMENT&);
		template<bool> void			Emit_MulTmp64RegReg(const STATEMENT&);

		//MOV
		void						Emit_Mov_RegRel(const STATEMENT&);
		void						Emit_Mov_RegReg(const STATEMENT&);
		void						Emit_Mov_RegCst(const STATEMENT&);
		void						Emit_Mov_RegTmp(const STATEMENT&);
		void						Emit_Mov_RelCst(const STATEMENT&);
		void						Emit_Mov_RelReg(const STATEMENT&);

		//JMP
		void						Emit_Jmp(const STATEMENT&);

		//CONDJMP
		void						Emit_CondJmp_RelCst(const STATEMENT&);
		void						Emit_CondJmp_RegCst(const STATEMENT&);

		//EXTLOW64
		void						Emit_ExtLow64RegTmp64(const STATEMENT&);

		//EXTHIGH64
		void						Emit_ExtHigh64RegTmp64(const STATEMENT&);
		void						Emit_ExtHigh64RelTmp64(const STATEMENT&);

		//FPUOP
		template <typename> void	Emit_Fpu_RelRel(const STATEMENT&);
		template <typename> void	Emit_Fpu_RelRelRel(const STATEMENT&);

		CX86Assembler				m_assembler;
		CX86Assembler::REGISTER*	m_registers;
		LabelMapType				m_labels;
		uint32						m_stackLevel;
		
		MatcherMapType				m_matchers;

	private:
		typedef void (CCodeGen_x86::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		static CONSTMATCHER			g_constMatchers[];
		static CONSTMATCHER			g_fpuConstMatchers[];
	};
}

#endif
