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
		virtual							~CCodeGen_Arm();

		void							GenerateCode(const StatementList&, unsigned int);
		void							SetStream(Framework::CStream*);
		unsigned int					GetAvailableRegisterCount() const;

	private:
		typedef std::map<uint32, CArmAssembler::LABEL> LabelMapType;
		typedef void (CCodeGen_Arm::*ConstCodeEmitterType)(const STATEMENT&);

		enum
		{
			MAX_REGISTERS = 6,
		};

		enum
		{
			LITERAL_POOL_SIZE = 0x40,
		};
		
		struct LITERAL_POOL_REF
		{
			unsigned int			poolPtr;
			CArmAssembler::REGISTER	dstRegister;
			unsigned int			offset;
		};

		typedef std::list<LITERAL_POOL_REF> LiteralPoolRefList;

		struct CONSTMATCHER
		{
			OPERATION					op;
			MATCHTYPE					dstType;
			MATCHTYPE					src1Type;
			MATCHTYPE					src2Type;
			ConstCodeEmitterType		emitter;
		};

		CArmAssembler::LABEL			GetLabel(uint32);
		void							MarkLabel(const STATEMENT&);

		uint32							RotateRight(uint32);
		uint32							RotateLeft(uint32);
		void							LoadConstantInRegister(CArmAssembler::REGISTER, uint32);
		void							DumpLiteralPool();

		void							Emit_Mov_RegCst(const STATEMENT&);
		void							Emit_Mov_RelReg(const STATEMENT&);

		static CONSTMATCHER				g_constMatchers[];
		static CArmAssembler::REGISTER	g_registers[MAX_REGISTERS];
		static CArmAssembler::REGISTER	g_baseRegister;

		Framework::CStream*				m_stream;
		CArmAssembler					m_assembler;
		LabelMapType					m_labels;
		uint32*							m_literalPool;
		unsigned int					m_lastLiteralPtr;
		LiteralPoolRefList				m_literalPoolRefs;
	};
};

#endif
