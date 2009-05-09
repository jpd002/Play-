#ifndef _CODEGENARM_H_
#define _CODEGENARM_H_

#include "CodeGenBase.h"
#include "ArmAssembler.h"
#include <list>
#include <functional>

class CCodeGen : public CCodeGenBase
{
public:
									CCodeGen();
	virtual							~CCodeGen();
	
	bool							AreAllRegistersFreed() const;

	virtual	void					Begin();
	virtual void					End();
	
	virtual void					EndQuota();
	
    virtual void                    BeginIf(bool);
    virtual void                    EndIf();
	
    virtual void                    BeginIfElse(bool);
    virtual void                    BeginIfElseAlt();

	virtual void					PushReg(unsigned int);
    virtual void                    PullRel(size_t);
	virtual void					PullTop();
	
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
	
    //FPU
    virtual void                    FP_PushWord(size_t);
    virtual void                    FP_PushSingle(size_t);
    virtual void                    FP_PullWordTruncate(size_t);
    virtual void                    FP_PullSingle(size_t);
    virtual void                    FP_PushCst(float);
	
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
	
private:
	enum
	{
		MAX_USABLE_REGISTERS = 6,
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
	typedef std::tr1::function<void (CArmAssembler::REGISTER, CArmAssembler::REGISTER, CArmAssembler::REGISTER)> TriRegOp;
	typedef std::tr1::function<uint32 (uint32, uint32)> ConstConstOp;
	
	void							GenericCommutativeOperation(const TriRegOp&, const ConstConstOp&);
	void							GenericConstantShift(CArmAssembler::SHIFT, uint8);
	void							GenericVariableShift(CArmAssembler::SHIFT);
	
	static uint32					Const_And(uint32, uint32);
	static uint32					Const_Or(uint32, uint32);
	static uint32					Const_Xor(uint32, uint32);
	
	CArmAssembler::REGISTER			AllocateRegister();
	void							FreeRegister(CArmAssembler::REGISTER);
	bool							RegisterHasNextUse(CArmAssembler::REGISTER);
	
	void							PushReg(CArmAssembler::REGISTER);
	
	void							LoadRelativeInRegister(CArmAssembler::REGISTER, uint32);
	void							LoadConstantInRegister(CArmAssembler::REGISTER, uint32);
	
	void							DumpLiteralPool();
	
	static CArmAssembler::REGISTER	g_baseRegister;
	static CArmAssembler::REGISTER	g_usableRegisters[MAX_USABLE_REGISTERS];
	bool							m_registerAllocated[MAX_USABLE_REGISTERS];
	CArmAssembler					m_assembler;
	uint32*							m_literalPool;
	unsigned int					m_lastLiteralPtr;
	LiteralPoolRefList				m_literalPoolRefs;
};

#endif
