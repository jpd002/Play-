#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "CacheBlock.h"
#include "ArrayStack.h"

namespace CodeGen
{
	class CFPU;
}

class CCodeGen
{
public:
	enum CONDITION
	{
		CONDITION_EQ,
		CONDITION_NE,
		CONDITION_BL,
		CONDITION_BE,
		CONDITION_LT,
		CONDITION_AB,
	};

	friend						CodeGen::CFPU;

	static void					Begin(CCacheBlock*);
	static void					End();

	static void					BeginIf(bool);
	static void					EndIf();

	static void					BeginIfElse(bool);
	static void					BeginIfElseAlt();

	static void					PushVar(uint32*);
	static void					PushCst(uint32);
	static void					PushRef(void*);

	static void					PullVar(uint32*);

	static void					Add();
	static void					Add64();
	static void					And();
	static void					Call(void*, unsigned int, bool);
	static void					Cmp(CONDITION);
	static void					Cmp64(CONDITION);
	static void					Lzc();
	static void					Or();
	static void					SeX();
	static void					SeX16();
	static void					Shl(uint8);
	static void					Shr64(uint8);
	static void					Sub();
	static void					Xor();

private:
	enum MAX_STACK
	{
		MAX_STACK = 0x100,
	};

	enum MAX_REGISTER
	{
		MAX_REGISTER = 6,
	};

	enum SYMBOLS
	{
		VARIABLE = 0x8000,
		REGISTER,
		CONSTANT,
		REFERENCE,
	};

	enum IFBLOCKS
	{
		IFBLOCK,
		IFELSEBLOCK,
	};

	enum REGISTER_TYPE
	{
		REGISTER_NORMAL,
		REGISTER_HASLOW,
	};

	static unsigned int			AllocateRegister(REGISTER_TYPE = REGISTER_NORMAL);
	static void					FreeRegister(unsigned int);
	static unsigned int			GetMinimumConstantSize(uint32);
	static bool					RegisterHasNextUse(unsigned int);
	static void					LoadVariableInRegister(unsigned int, uint32);

	static bool					IsTopRegCstPairCom();
	static void					GetRegCstPairCom(unsigned int*, uint32*);

	static void					PushReg(unsigned int);

	static void					Cmp64Eq();
	static void					Cmp64Lt(bool);

	static bool					m_nBlockStarted;

	static CArrayStack<uint32>	m_Shadow;
	static bool					m_nRegisterAllocated[MAX_REGISTER];
	static unsigned int			m_nRegisterLookup[MAX_REGISTER];
	static CCacheBlock*			m_pBlock;

	static CArrayStack<uint32>	m_IfStack;
};

#endif
