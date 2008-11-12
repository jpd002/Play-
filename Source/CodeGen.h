#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "ArrayStack.h"
#include "X86Assembler.h"
#include "Stream.h"

#ifdef WIN32
#undef RELATIVE
#undef CONDITION
#endif

class CCodeGen
{
public:
    typedef CX86Assembler::XMMREGISTER XMMREGISTER;

    struct GenericOp
    {
        typedef void (CX86Assembler::*RegisterAddressOpPtr)(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
        typedef void (CX86Assembler::*AddressConstantOpPtr)(const CX86Assembler::CAddress&, uint32);
        typedef uint32 (*ConstantConstantOpPtr)(uint32, uint32);
        typedef bool (*IsNullOpPtr)(uint32);

        GenericOp(
            const RegisterAddressOpPtr& registerAddressOp, 
            const AddressConstantOpPtr& addressConstantOp,
            const ConstantConstantOpPtr& constantConstantOp,
            const IsNullOpPtr& isNullOp)
            : 
            registerAddressOp(registerAddressOp), 
            addressConstantOp(addressConstantOp),
            constantConstantOp(constantConstantOp),
            isNullOp(isNullOp)
            {}

        RegisterAddressOpPtr    registerAddressOp;
        AddressConstantOpPtr    addressConstantOp;
        ConstantConstantOpPtr   constantConstantOp;
        IsNullOpPtr             isNullOp;
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

                                    CCodeGen();
    virtual                         ~CCodeGen();

    void                            Begin();
    void                            End();

    bool                            IsStackEmpty();
	virtual void					EndQuota();
	
    virtual void                    BeginIf(bool);
    virtual void                    EndIf();

    virtual void                    BeginIfElse(bool);
    virtual void                    BeginIfElseAlt();

    void                            PushCst(uint32);
    void                            PushRef(void*);
    virtual void                    PushRel(size_t);
    void                            PushTop();
    void                            PushIdx(unsigned int);

    virtual void                    PullRel(size_t);
    void                            PullTop();
    void                            Swap();

    void                            Add();
    void						    Add64();
    void                            And();
    void                            And64();
    virtual void                    Call(void*, unsigned int, bool);
    void                            Cmp(CONDITION);
    void                            Cmp64(CONDITION);
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
    void                            Shl64();
    void                            Shl64(uint8);
    void                            Sra();
    void                            Sra(uint8);
    void                            Sra64(uint8);
    void                            Srl();
    void                            Srl(uint8);
    void                            Srl64();
    void                            Srl64(uint8);
    void                            Sub();
    void                            Sub64();
    void                            Xor();

    void                            GenericLogical(const GenericOp&);

    //FPU
    virtual void                    FP_PushWord(size_t);
    virtual void                    FP_PushSingle(size_t);
    virtual void                    FP_PullWordTruncate(size_t);
    virtual void                    FP_PullSingle(size_t);
    virtual void                    FP_PushCst(float);
    void                            FP_PushSingleReg(XMMREGISTER);
    void                            FP_LoadSingleRelativeInRegister(XMMREGISTER, uint32);

    void                            FP_Add();
    void                            FP_Abs();
    void                            FP_Sub();
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
    void                            MD_PushReg(XMMREGISTER);

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
    CX86Assembler                   m_Assembler;

protected:
    CArrayStack<uint32>	            m_Shadow;
    CArrayStack<uint32>             m_IfStack;
    void                            PushReg(unsigned int);
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

    unsigned int                    AllocateRegister(REGISTER_TYPE = REGISTER_NORMAL);
	void                            FreeRegister(unsigned int);
	static unsigned int				GetMinimumConstantSize(uint32);
    static unsigned int             GetMinimumConstantSize64(uint64);
	bool                            RegisterHasNextUse(unsigned int);
    bool                            Register128HasNextUse(XMMREGISTER);
    bool                            RegisterFpSingleHasNextUse(XMMREGISTER);
    void                            LoadRelativeInRegister(unsigned int, uint32);
    void                            LoadConstantInRegister(unsigned int, uint32);
    void                            LoadRelative128InRegister(XMMREGISTER, uint32);
	void                            CopyRegister(unsigned int, unsigned int);
    void                            CopyRegister128(XMMREGISTER, XMMREGISTER);
#ifdef AMD64
	static void						LoadRelativeInRegister64(unsigned int, uint32);
	static void						LoadConstantInRegister64(unsigned int, uint64);
#endif

    XMMREGISTER                     AllocateXmmRegister();
    void                            FreeXmmRegister(XMMREGISTER);

    void                            ReduceToRegister();

    bool                            IsTopRegZeroPairCom();
    bool                            IsTopContRelPair64();
    bool                            IsTopContRelCstPair64();
    void                            GetRegCstPairCom(unsigned int*, uint32*);
    
    template <typename T> 
    void UnaryRelativeSelfCallAsRegister(const T& Function)
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
    bool FitsPattern()
    {
        return StackPattern().Fits(m_Shadow);
    }

    template <typename StackPattern>
    typename StackPattern::PatternValue GetPattern()
    {
        return StackPattern().Get(m_Shadow);
    }

#ifdef AMD64
	static void						PushReg64(unsigned int);
#endif
	void                            ReplaceRegisterInStack(unsigned int, unsigned int);

	void                            Cmp64Eq();
	void                            Cmp64Lt(bool, bool);
    void                            Cmp64Cont(CONDITION);

    typedef std::tr1::function<void (const CX86Assembler::CAddress&)> MultFunction;
    typedef std::tr1::function<void (XMMREGISTER, uint8)> PackedShiftFunction;
    typedef std::tr1::function<void (XMMREGISTER, const CX86Assembler::CAddress&)> MdTwoOperandFunction;

    void                            Div_Base(const MultFunction&, bool);
    void                            Mult_Base(const MultFunction&, bool);

    XMMREGISTER                     FP_GetResultRegister(uint32);
    void                            FP_GenericOneOperand(const MdTwoOperandFunction&);
    void                            FP_GenericTwoOperand(const MdTwoOperandFunction&);

    void                            FP_CmpHelper(XMMREGISTER, const CX86Assembler::CAddress&, CCodeGen::CONDITION);
    void                            FP_GenericNeg(XMMREGISTER, const CX86Assembler::CAddress&);

    void                            MD_GenericPackedShift(const PackedShiftFunction&, uint8);
    void                            MD_GenericOneOperand(const MdTwoOperandFunction&);
    void                            MD_GenericTwoOperand(const MdTwoOperandFunction&);
    void                            MD_GenericTwoOperandReversed(const MdTwoOperandFunction&);

    void                            StreamWriteByte(uint8);
    void                            StreamWriteAt(unsigned int, uint8);
    size_t                          StreamTell();

    void                            EmitLoad(uint32, uint32, unsigned int);
    void                            EmitOp(const GenericOp&, uint32, uint32, unsigned int);

    bool						    m_nBlockStarted;

#ifdef AMD64
	static CArrayStack<uint32, 2>	m_PullReg64Stack;
#endif
	bool						    m_nRegisterAllocated[MAX_REGISTER];
	static unsigned int				m_nRegisterLookup[MAX_REGISTER];
    static CX86Assembler::REGISTER  m_nRegisterLookupEx[MAX_REGISTER];
    bool                            m_xmmRegisterAllocated[MAX_XMM_REGISTER];

    Framework::CStream*             m_stream;
    static CX86Assembler::REGISTER  g_nBaseRegister;
};

#endif
