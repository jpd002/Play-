#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "CacheBlock.h"
#include "ArrayStack.h"
#include "X86Assembler.h"
#include "Stream.h"

#ifdef WIN32
#undef RELATIVE
#undef CONDITION
#endif

namespace CodeGen
{
	class CFPU;
}

class CCodeGen
{
public:
    typedef CX86Assembler::XMMREGISTER XMMREGISTER;

    struct GenericOp
    {
        typedef void (CX86Assembler::*RelativeOpPtr)(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
        typedef void (CX86Assembler::*ConstantOpPtr)(const CX86Assembler::CAddress&, uint32);

        GenericOp(const RelativeOpPtr& relativeOp, const ConstantOpPtr& constantOp)
            : relativeOp(relativeOp), constantOp(constantOp) {}

        ConstantOpPtr   constantOp;
        RelativeOpPtr   relativeOp;
    };

    enum CONDITION
	{
		CONDITION_EQ,
		CONDITION_NE,
		CONDITION_BL,
		CONDITION_BE,
		CONDITION_LT,
		CONDITION_AB,
		CONDITION_GT,
		CONDITION_LE,
	};

	enum SYMBOLS
	{
		VARIABLE = 0x8000,
		REGISTER,
		CONSTANT,
		REFERENCE,
		RELATIVE,
#ifdef AMD64
		REGISTER64,
#endif
        FP_SINGLE_RELATIVE,
        FP_SINGLE_REGISTER,
        RELATIVE128,
        REGISTER128,
	};

	enum ROUNDMODE
	{
		ROUND_NEAREST = 0,
		ROUND_PLUSINFINITY = 1,
		ROUND_MINUSINFINITY = 2,
		ROUND_TRUNCATE = 3
	};

    friend class					CodeGen::CFPU;

                                    CCodeGen();
    virtual                         ~CCodeGen();

	static void						Begin(CCacheBlock*);
	static void						End();

    bool                            IsStackEmpty();
	virtual void					EndQuota();
	
    static void						BeginIf(bool);
	static void						EndIf();

	static void						BeginIfElse(bool);
	static void						BeginIfElseAlt();

	static void						PushVar(uint32*);
	static void						PushCst(uint32);
	static void						PushRef(void*);
	static void						PushRel(size_t);
	static void						PushTop();
	static void						PushIdx(unsigned int);

	static void						PullVar(uint32*);
	static void						PullRel(size_t);
	static void						PullTop();
    void                            Swap();

	static void						Add();
	void						    Add64();
	static void						And();
    void                            And64();
	static void						Call(void*, unsigned int, bool);
	static void						Cmp(CONDITION);
	static void						Cmp64(CONDITION);
    void                            Div();
    void                            DivS();
    void                            Lookup(uint32*);
    static void						Lzc();
    void                            Mult();
    void                            MultS();
    void                            Not();
    void                            Or();
	static void						SeX();
    void                            SeX8();
	static void						SeX16();
    void                            Shl();
	void                            Shl(uint8);
    void                            Shl64();
	void                            Shl64(uint8);
    void                            Sra();
    void                            Sra(uint8);
    void                            Sra64(uint8);
    void                            Srl();
    void                            Srl(uint8);
	void                            Srl64();
	void                            Srl64(uint8);
    static void                     Sub();
    void                            Sub64();
	static void						Xor();

    //FPU
    virtual void                    FP_PushWord(size_t);
    virtual void                    FP_PushSingle(size_t);
    virtual void                    FP_PullWordTruncate(size_t);
    virtual void                    FP_PullSingle(size_t);
    void                            FP_PushSingleReg(XMMREGISTER);
    void                            FP_LoadSingleRelativeInRegister(XMMREGISTER, uint32);

    void                            FP_Add();
    void                            FP_Sub();
    void                            FP_Mul();
    void                            FP_Div();
    void                            FP_Cmp(CONDITION);
    void                            FP_Neg();

    //SIMD (128-bits only)
    virtual void                    MD_PushRel(size_t);
    virtual void                    MD_PullRel(size_t);
    void                            MD_PushReg(XMMREGISTER);

    void                            MD_AddWUS();
    void                            MD_And();
    void                            MD_MaxH();
    void                            MD_MinH();
    void                            MD_Not();
    void                            MD_Or();
    void                            MD_PackHB();
    void                            MD_SubB();
    void                            MD_SubW();
    void                            MD_Xor();

    void                            SetStream(Framework::CStream*);
    static CX86Assembler            m_Assembler;

protected:
    static CArrayStack<uint32>		m_Shadow;
	static CArrayStack<uint32>		m_IfStack;
	static void						PushReg(unsigned int);
    static bool                     IsRegisterSaved(unsigned int);

private:
	enum MAX_STACK
	{
		MAX_STACK = 0x100,
	};

	enum MAX_REGISTER
	{
#ifdef AMD64
		MAX_REGISTER = 14,
#else
		MAX_REGISTER = 6,
#endif
	};

    enum MAX_XMM_REGISTER
    {
#ifdef AMD64
        MAX_XMM_REGISTER = 16,
#else
        MAX_XMM_REGISTER = 8,
#endif
    };

	enum REL_REGISTER
	{
		REL_REGISTER = 5,
	};

	enum IFBLOCKS
	{
		IFBLOCK,
		IFELSEBLOCK,
	};

    enum SPECREGS
    {
        REGISTER_EAX = 0,
        REGISTER_EDX = 2,
    };

	enum REGISTER_TYPE
	{
		REGISTER_NORMAL,
		REGISTER_HASLOW,
		REGISTER_SAVED,
		REGISTER_SHIFTAMOUNT,
	};

    static unsigned int				AllocateRegister(REGISTER_TYPE = REGISTER_NORMAL);
	static void						FreeRegister(unsigned int);
	static unsigned int				GetMinimumConstantSize(uint32);
    static unsigned int             GetMinimumConstantSize64(uint64);
	static bool						RegisterHasNextUse(unsigned int);
    bool                            Register128HasNextUse(XMMREGISTER);
    bool                            RegisterFpSingleHasNextUse(XMMREGISTER);
	static void						LoadVariableInRegister(unsigned int, uint32);
	static void						LoadRelativeInRegister(unsigned int, uint32);
	static void						LoadConstantInRegister(unsigned int, uint32);
    void                            LoadRelative128InRegister(XMMREGISTER, uint32);
	static void						CopyRegister(unsigned int, unsigned int);
#ifdef AMD64
	static void						LoadRelativeInRegister64(unsigned int, uint32);
	static void						LoadConstantInRegister64(unsigned int, uint64);
#endif

    XMMREGISTER                     AllocateXmmRegister();
    void                            FreeXmmRegister(XMMREGISTER);

	static void						LoadConditionInRegister(unsigned int, CONDITION);

	static void						ReduceToRegister();

	static bool						IsTopRegZeroPairCom();
    static bool                     IsTopContRelPair64();
    static bool                     IsTopContRelCstPair64();
	static void						GetRegCstPairCom(unsigned int*, uint32*);
    
    template <typename T> 
    static void                     UnaryRelativeSelfCallAsRegister(const T& Function)
    {
		uint32 nRelative;
		unsigned int nRegister;

		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		Function();
    }

    template <typename StackPattern>
    static bool                     FitsPattern()
    {
        return StackPattern().Fits(m_Shadow);
    }

    template <typename StackPattern>
    static typename StackPattern::PatternValue GetPattern()
    {
        return StackPattern().Get(m_Shadow);
    }

#ifdef AMD64
	static void						PushReg64(unsigned int);
#endif
	static void						ReplaceRegisterInStack(unsigned int, unsigned int);

	static void						Cmp64Eq();
	static void						Cmp64Lt(bool, bool);
    static void                     Cmp64Cont(CONDITION);

    typedef std::tr1::function<void (const CX86Assembler::CAddress&)> MultFunction;

    void                            Div_Base(const MultFunction&);
    void                            Mult_Base(const MultFunction&, bool);

    static void                     StreamWriteByte(uint8);
    static void                     StreamWriteAt(unsigned int, uint8);
    static size_t                   StreamTell();

    static void                     EmitLoad(uint32, uint32, unsigned int);
    static void                     EmitOp(const GenericOp&, uint32, uint32, unsigned int);

    static bool						m_nBlockStarted;

#ifdef AMD64
	static CArrayStack<uint32, 2>	m_PullReg64Stack;
#endif
	static bool						m_nRegisterAllocated[MAX_REGISTER];
	static unsigned int				m_nRegisterLookup[MAX_REGISTER];
    static CX86Assembler::REGISTER  m_nRegisterLookupEx[MAX_REGISTER];
	static CCacheBlock*				m_pBlock;
    static bool                     m_xmmRegisterAllocated[MAX_XMM_REGISTER];

    static Framework::CStream*      m_stream;
    static CX86Assembler::REGISTER  g_nBaseRegister;
};

#endif
