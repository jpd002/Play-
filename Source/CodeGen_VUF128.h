#ifndef _CODEGEN_VUF128_H_
#define _CODEGEN_VUF128_H_

#include "CodeGen.h"
#include "uint128.h"

namespace CodeGen
{

	class CVUF128
	{
		friend static void				CCodeGen::Begin(CCacheBlock*);
		friend static void				CCodeGen::End();

	public:
		static void						Push(uint128*);
		static void						Push(uint32*);
		static void						PushImm(float);
		static void						PushTop();
		static void						Pull(uint32*, uint32*, uint32*, uint32*);

		static void						Add();
		static void						Max();
		static void						Min();
		static void						Mul();
		static void						Sub();
		static void						Truncate();
		static void						IsZero();
		static void						IsNegative();
		static void						ToWordTruncate();
		static void						ToSingle();

	private:
		class CFactory
		{
		public:
			virtual						~CFactory();
			virtual void				Begin()														= 0;
			virtual void				End(CCacheBlock*)											= 0;
			virtual void				Push(CCacheBlock*, uint128*)								= 0;
			virtual void				Push(CCacheBlock*, uint32*)									= 0;
			virtual void				PushImm(CCacheBlock*, float)								= 0;
			virtual void				PushTop(CCacheBlock*)										= 0;
			virtual void				Pull(CCacheBlock*, uint32*, uint32*, uint32*, uint32*)		= 0;

			virtual void				Add(CCacheBlock*)											= 0;
			virtual void				Max(CCacheBlock*)											= 0;
			virtual void				Min(CCacheBlock*)											= 0;
			virtual void				Mul(CCacheBlock*)											= 0;
			virtual void				Sub(CCacheBlock*)											= 0;
			virtual void				Truncate(CCacheBlock*)										= 0;
			virtual void				IsZero(CCacheBlock*)										= 0;
			virtual void				IsNegative(CCacheBlock*)									= 0;
			virtual void				ToWordTruncate(CCacheBlock*)								= 0;
			virtual void				ToSingle(CCacheBlock*)										= 0;
		};

		class CSSEFactory : public CFactory
		{
		public:
										CSSEFactory();
			virtual						~CSSEFactory();
			virtual void				Begin();
			virtual void				End(CCacheBlock*);
			virtual void				Push(CCacheBlock*, uint128*);
			virtual void				Push(CCacheBlock*, uint32*);
			virtual void				PushImm(CCacheBlock*, float);
			virtual void				PushTop(CCacheBlock*);
			virtual void				Pull(CCacheBlock*, uint32*, uint32*, uint32*, uint32*);

			virtual void				Add(CCacheBlock*);
			virtual void				Max(CCacheBlock*);
			virtual void				Min(CCacheBlock*);
			virtual void				Mul(CCacheBlock*);
			virtual void				Sub(CCacheBlock*);
			virtual void				Truncate(CCacheBlock*);
			virtual void				IsZero(CCacheBlock*);
			virtual void				IsNegative(CCacheBlock*);
			virtual void				ToWordTruncate(CCacheBlock*);
			virtual void				ToSingle(CCacheBlock*);

		protected:
			void						ReserveStack(CCacheBlock*, uint8);
			void						RequireEMMS();

			unsigned int				m_nRegister;
			unsigned int				m_nStackAlloc;
			bool						m_nNeedsEMMS;
		};

		class CSSE2Factory : public CSSEFactory
		{
		public:
										CSSE2Factory();
			virtual						~CSSE2Factory();
			virtual void				IsZero(CCacheBlock*);
			virtual void				IsNegative(CCacheBlock*);
			virtual void				ToWordTruncate(CCacheBlock*);
			virtual void				ToSingle(CCacheBlock*);
		};

		static void						Begin(CCacheBlock*);
		static void						End();

		static CFactory*				CreateFactory();

		static CFactory*				m_pFactory;

		static CCacheBlock*				m_pBlock;
		static uint8					m_nRegister;
	};

}

#endif
