#ifndef _CODEGEN_VUI128_H_
#define _CODEGEN_VUI128_H_

#include "CodeGen.h"
#include "ArrayStack.h"
#include "uint128.h"

namespace CodeGen
{

	class CVUI128
	{
		friend void						CCodeGen::Begin(CCacheBlock*);
		friend void						CCodeGen::End();

	public:
		static void						Push(uint128*);
		static void						Pull(uint128*);

		static void						AddH();
		static void						AddWSS();
		static void						AddWUS();
		static void						And();
		static void						CmpEqW();
		static void						CmpGtH();
		static void						MaxH();
		static void						MinH();
		static void						Not();
		static void						Or();
		static void						PackHB();
		static void						PackWH();
		static void						SubB();
		static void						SubW();
		static void						SllH(unsigned int);
		static void						SrlH(unsigned int);
		static void						SraH(unsigned int);
		static void						SrlVQw(uint32*);
		static void						UnpackLowerBH();
		static void						UnpackUpperBH();
		static void						UnpackLowerHW();
		static void						UnpackLowerWD();
		static void						UnpackUpperWD();
		static void						Xor();

	private:
		enum SYMBOLS
		{
			VARIABLE,
			REGISTER,
			STACK,
		};

		class CFactory
		{
		public:
			virtual						~CFactory();
			virtual void				Begin(CCacheBlock*)	= 0;
			virtual void				End()				= 0;

			virtual void				Pull(uint128*)		= 0;

			virtual void				AddH()				= 0;
			virtual void				And()				= 0;
			virtual void				CmpEqW()			= 0;
			virtual void				CmpGtH()			= 0;
			virtual void				MaxH()				= 0;
			virtual void				MinH()				= 0;
			virtual void				Not()				= 0;
			virtual void				Or()				= 0;
			virtual void				PackHB()			= 0;
			virtual void				SubB()				= 0;
			virtual void				SubW()				= 0;
			virtual void				SllH(unsigned int)	= 0;
			virtual void				SrlH(unsigned int)	= 0;
			virtual void				SraH(unsigned int)	= 0;
			virtual void				UnpackLowerBH()		= 0;
			virtual void				UnpackUpperBH()		= 0;
			virtual void				UnpackLowerHW()		= 0;
			virtual void				UnpackLowerWD()		= 0;
			virtual void				UnpackUpperWD()		= 0;
			virtual void				Xor()				= 0;
		};

		class CSSEFactory : public CFactory
		{
		public:
			virtual void				Begin(CCacheBlock*);
			virtual void				End();

			virtual void				Pull(uint128*);

			virtual void				AddH();
			virtual void				And();
			virtual void				CmpEqW();
			virtual void				CmpGtH();
			virtual void				MaxH();
			virtual void				MinH();
			virtual void				Not();
			virtual void				Or();
			virtual void				PackHB();
			virtual void				SubB();
			virtual void				SubW();
			virtual void				SllH(unsigned int);
			virtual void				SrlH(unsigned int);
			virtual void				SraH(unsigned int);
			virtual void				UnpackLowerBH();
			virtual void				UnpackUpperBH();
			virtual void				UnpackLowerHW();
			virtual void				UnpackLowerWD();
			virtual void				UnpackUpperWD();
			virtual void				Xor();

		private:
			enum MAX_REGISTER
			{
				MAX_REGISTER = 4,
			};

			unsigned int				AllocateRegister();
			void						FreeRegister(unsigned int);
			void						BeginTwoOperands(uint32*, uint32*);
			void						EndTwoOperands(uint32);
			void						LoadVariableInRegister(uint32, unsigned int);
			void						RequireEMMS();

			bool						m_nRequiresEMMS;
			CCacheBlock*				m_pBlock;
			bool						m_nRegisterAllocated[MAX_REGISTER];
		};

		class CSSE2Factory : public CFactory
		{
		public:
			virtual void				Begin(CCacheBlock*);
			virtual void				End();

			virtual void				Pull(uint128*);

			virtual void				AddH();
			virtual void				And();
			virtual void				CmpEqW();
			virtual void				CmpGtH();
			virtual void				MaxH();
			virtual void				MinH();
			virtual void				Not();
			virtual void				Or();
			virtual void				PackHB();
			virtual void				SubB();
			virtual void				SubW();
			virtual void				SllH(unsigned int);
			virtual void				SrlH(unsigned int);
			virtual void				SraH(unsigned int);
			virtual void				UnpackLowerBH();
			virtual void				UnpackUpperBH();
			virtual void				UnpackLowerHW();
			virtual void				UnpackLowerWD();
			virtual void				UnpackUpperWD();
			virtual void				Xor();

		private:
			enum MAX_REGISTER
			{
				MAX_REGISTER = 8,
			};

			unsigned int				AllocateRegister();
			void						FreeRegister(unsigned int);
			void						BeginTwoOperands(uint32*, uint32*);
			void						EndTwoOperands(uint32);
			void						LoadVariableInRegister(uint32, unsigned int);

			CCacheBlock*				m_pBlock;
			bool						m_nRegisterAllocated[MAX_REGISTER];
		};

		static CFactory*				CreateFactory();

		static void						Begin(CCacheBlock*);
		static void						End();

		static CArrayStack<uint32>		m_OpStack;
		static CFactory*				m_pFactory;
		static CCacheBlock*				m_pBlock;
	};

}

#endif
