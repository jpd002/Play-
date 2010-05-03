#ifndef _JITTER_H_
#define _JITTER_H_

#include <string>
#include <memory>
#include <list>
#include <map>
#include <boost/lexical_cast.hpp>
#include "ArrayStack.h"
#include "Stream.h"
#include "Jitter_SymbolTable.h"
#include "Jitter_CodeGen.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#include "Jitter_Statement.h"

namespace Jitter
{

	class CJitter
	{
	public:
		enum ROUNDMODE
		{
			ROUND_NEAREST = 0,
			ROUND_PLUSINFINITY = 1,
			ROUND_MINUSINFINITY = 2,
			ROUND_TRUNCATE = 3
		};

										CJitter(CCodeGen*);
		virtual                         ~CJitter();

		void                            Begin();
		void                            End();

		bool                            IsStackEmpty();

		virtual void					BeginIf(CONDITION);
		virtual void					Else();
		virtual void					EndIf();

		void                            PushCst(uint32);
		void                            PushRef(void*);
		virtual void                    PushRel(size_t);
		void                            PushTop();
		void                            PushIdx(unsigned int);

		virtual void                    PullRel(size_t);
		void                            PullTop();
		void                            Swap();

		void                            Add();
		void                            And();
		virtual void                    Call(void*, unsigned int, bool);
		void                            Cmp(CONDITION);
		void                            Div();
		void                            DivS();
		void                            Lookup(uint32*);
		void                            Lzc();
		void                            Mult();
		void                            MultS();
		void                            Not();
		void                            Or();
		void                            SeX();
		void                            SeX8();
		void                            SeX16();
		void                            Shl();
		void                            Shl(uint8);
		void                            Sra();
		void                            Sra(uint8);
		void                            Srl();
		void                            Srl(uint8);
		void                            Sub();
		void                            Xor();

		//64-bits
		void							PushRel64(size_t);
		void							PullRel64(size_t);
		void							ExtLow64();
		void							ExtHigh64();

		void							Add64();
		void                            Sub64();
		void                            And64();
		void                            Cmp64(CONDITION);
		void                            Srl64();
		void                            Srl64(uint8);
		void                            Sra64(uint8);
		void                            Shl64();
		void                            Shl64(uint8);

		//FPU
		virtual void                    FP_PushWord(size_t);
		virtual void                    FP_PushSingle(size_t);
		virtual void                    FP_PullWordTruncate(size_t);
		virtual void                    FP_PullSingle(size_t);
		virtual void                    FP_PushCst(float);

		void                            FP_Add();
		void                            FP_Abs();
		void                            FP_Sub();
		void                            FP_Max();
		void                            FP_Min();
		void                            FP_Mul();
		void                            FP_Div();
		void                            FP_Cmp(CONDITION);
		void                            FP_Neg();
		void                            FP_Rcpl();
		void                            FP_Sqrt();
		void                            FP_Rsqrt();

		//SIMD (128-bits only)
		virtual void                    MD_PushRel(size_t);
		virtual void                    MD_PushRelExpand(size_t);
		void                            MD_PushCstExpand(float);
		virtual void                    MD_PullRel(size_t);
		virtual void                    MD_PullRel(size_t, size_t, size_t, size_t);
		virtual void                    MD_PullRel(size_t, bool, bool, bool, bool);

		void                            MD_AbsS();
		void                            MD_AddH();
		void                            MD_AddWSS();
		void                            MD_AddWUS();
		void                            MD_AddS();
		void                            MD_And();
		void                            MD_CmpEqW();
		void                            MD_CmpGtH();
		void                            MD_DivS();
		void                            MD_IsNegative();
		void                            MD_IsZero();
		void                            MD_MaxH();
		void                            MD_MaxS();
		void                            MD_MinH();
		void                            MD_MinS();
		void                            MD_MulS();
		void                            MD_Not();
		void                            MD_Or();
		void                            MD_PackHB();
		void                            MD_PackWH();
		void                            MD_SllH(uint8);
		void                            MD_SraH(uint8);
		void                            MD_SraW(uint8);
		void                            MD_SrlH(uint8);
		void                            MD_Srl256();
		void                            MD_SubB();
		void                            MD_SubW();
		void                            MD_SubS();
		void                            MD_ToSingle();
		void                            MD_ToWordTruncate();
		void                            MD_UnpackLowerBH();
		void                            MD_UnpackLowerHW();
		void                            MD_UnpackLowerWD();
		void                            MD_UnpackUpperBH();
		void                            MD_UnpackUpperWD();
		void                            MD_Xor();

		void                            SetStream(Framework::CStream*);

	protected:
		CArrayStack<SymbolPtr>			m_Shadow;
		CArrayStack<uint32>				m_IfStack;

		void							PushTmp64(unsigned int);

	private:
		enum MAX_STACK
		{
			MAX_STACK = 0x100,
		};

		enum IFBLOCKS
		{
			IFBLOCK,
			IFELSEBLOCK,
		};

		class CRelativeVersionManager
		{
		public:

			unsigned int				GetRelativeVersion(uint32);
			unsigned int				IncrementRelativeVersion(uint32);

		private:
			typedef std::map<uint32, unsigned int> RelativeVersionMap;

			RelativeVersionMap			m_relativeVersions;
		};

		struct BASIC_BLOCK
		{
			BASIC_BLOCK() : optimized(false), hasJumpRef(false) { }

			StatementList				statements;
			CSymbolTable				symbolTable;
			bool						optimized;
			bool						hasJumpRef;
		};
		typedef std::map<unsigned int, BASIC_BLOCK> BasicBlockList;

		struct VERSIONED_STATEMENT_LIST
		{
			StatementList				statements;
			CRelativeVersionManager		relativeVersions;
		};

		void							Compile();

		bool							ConstantFolding(StatementList&);
		bool							ConstantPropagation(StatementList&);
		bool							CopyPropagation(StatementList&);
		bool							DeadcodeElimination(VERSIONED_STATEMENT_LIST&);

		bool							FoldConstantOperation(STATEMENT&);
		bool							FoldConstant64Operation(STATEMENT&);

		BASIC_BLOCK						ConcatBlocks(const BasicBlockList&);
		bool							MergeBlocks();
		bool							PruneBlocks();
		void							HarmonizeBlocks();
		void							MergeBasicBlocks(BASIC_BLOCK&, const BASIC_BLOCK&);

		uint32							CreateBlock();
		BASIC_BLOCK*					GetBlock(uint32);

		void							InsertStatement(const STATEMENT&);

		SymbolPtr						MakeSymbol(SYM_TYPE, uint32);
		SymbolPtr						MakeSymbol(BASIC_BLOCK*, SYM_TYPE, uint32, uint32);
		SymbolPtr						MakeConstant64(uint64);

		SymbolRefPtr					MakeSymbolRef(const SymbolPtr&);
		int								GetSymbolSize(const SymbolRefPtr&);

		static CONDITION				GetReverseCondition(CONDITION);

		void							DumpStatementList(const StatementList&);
		VERSIONED_STATEMENT_LIST		GenerateVersionedStatementList(const StatementList&);
		StatementList					CollapseVersionedStatementList(const VERSIONED_STATEMENT_LIST&);
		void							ComputeLivenessAndPruneSymbols(BASIC_BLOCK&);
		void							AllocateRegisters(BASIC_BLOCK&);
		unsigned int					AllocateStack(BASIC_BLOCK&);

		bool						    m_nBlockStarted;

		unsigned int					m_nextTemporary;
		unsigned int					m_nextBlockId;

		BASIC_BLOCK*					m_currentBlock;
		BasicBlockList					m_basicBlocks;
		CCodeGen*						m_codeGen;
	};

}

#endif
