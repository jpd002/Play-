#ifndef _CODEGEN_FPU_H_
#define _CODEGEN_FPU_H_

#include "CodeGen.h"

namespace CodeGen
{

	class CFPU
	{
		friend void		CCodeGen::Begin(CCacheBlock*);
		friend void		CCodeGen::End();

	public:
		enum ROUNDMODE
		{
			ROUND_NEAREST = 0,
			ROUND_PLUSINFINITY = 1,
			ROUND_MINUSINFINITY = 2,
			ROUND_TRUNCATE = 3
		};

		static void				PushSingle(void*);
		static void				PushSingleImm(float);
		static void				PushWord(void*);

		static void				PullSingle(void*);
		static void				PullWord(void*);

		static void				PushRoundingMode();
		static void				PullRoundingMode();
		static void				SetRoundingMode(ROUNDMODE);

		static void				Add();
		static void				Abs();
		static void				Dup();
		static void				Div();
		static void				Sub();
		static void				Mul();
		static void				Neg();
		static void				Rcpl();
		static void				Sqrt();
		static void				Cmp(CCodeGen::CONDITION);
		static void				Round();

	private:
		static void				Begin(CCacheBlock*);
		static void				End();

		static CCacheBlock*		m_pBlock;
	};

}

#endif
