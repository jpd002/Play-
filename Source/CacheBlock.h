#ifndef _CACHEBLOCK_H_
#define _CACHEBLOCK_H_

#include "Types.h"

class CMIPS;

enum JCC_CONDITION
{
	JCC_CONDITION_ALWAYS,
	JCC_CONDITION_NE,
	JCC_CONDITION_EQ,
	JCC_CONDITION_LE,
	JCC_CONDITION_GE,
	JCC_CONDITION_LT,
	JCC_CONDITION_GT,
	JCC_CONDITION_BE,
	JCC_CONDITION_BL,
};

enum RET_CODE
{
	RET_CODE_QUOTADONE,
	RET_CODE_BREAKPOINT,
	RET_CODE_INTERBLOCKJUMP,
    RET_CODE_EXCEPTION,
};

class CCacheBlock
{
public:
	class CVUI128;
						CCacheBlock(uint32, uint32);
						~CCacheBlock();

	void				Compile(CMIPS*);
	RET_CODE			Execute(CMIPS*);

	void				StreamWriteByte(uint8);
	void				StreamWriteWord(uint32);
	void				StreamWrite(unsigned int, ...);
	void				StreamWriteAt(unsigned int, uint8);
	unsigned int		StreamGetSize();

	//Intermediate Language
	void				PushAddr(void*);
	void				PullAddr(void*);
	void				PushImm(uint32);
	void				PushRef(void*);
	void				SeX8();
	void				SeX16();
	void				SeX32();
	void				PushTop();
	void				PushValueAt(unsigned int);
	void				PullTop();
	void				Free(unsigned int);
	void				Lookup(uint32*);
	void				Exchange(unsigned int, unsigned int);
	void				Swap();
	void				And();
	void				AndImm(uint32);
	void				Add();
	void				AddC();
	void				AddImm(uint32);
	void				Sub();
	void				SubC();
	void				SubImm(uint32);
	void				Cmp(JCC_CONDITION);
	void				Call(void*, unsigned int, bool);
	void				BeginJcc(bool);
	void				EndJcc();
	void				Mult();
	void				MultS();
	void				Div();
	void				DivS();
	void				Or();
	void				OrImm(uint32);
	void				Xor();
	void				Not();
	void				Srl();
	void				SrlImm(uint8);
	void				Srl64();
	void				Sra();
	void				SraImm(uint8);
	void				Sra64();
	void				Sll();
	void				SllImm(uint8);
	void				Sll64();
	void				SetDelayedJumpCheck();
	void				SetProgramCounterChanged();

	//Proxies
	static uint32		GetByteProxy(CMIPS*, uint32);
	static uint32		GetHalfProxy(CMIPS*, uint32);
	static uint32		GetWordProxy(CMIPS*, uint32);
	static void			SetByteProxy(CMIPS*, uint32, uint32);
	static void			SetHalfProxy(CMIPS*, uint32, uint32);
	static void			SetWordProxy(CMIPS*, uint32, uint32);

private:
	void				Ret(RET_CODE);
	void				InsertProlog(CMIPS*);
	void				InsertEpilog(CMIPS*, bool);
	void				InsertPostCode(CMIPS*);
	void				SynchornizePC(CMIPS*);
	void				PushConditionBit(JCC_CONDITION);

	bool				m_nValid;
	bool				m_nCheckDelayedJump;
	bool				m_nProgramCounterChanged;

	uint32				m_nStart;
	uint32				m_nEnd;

	uint32				m_nPtr;
	uint32				m_nSize;
	uint8*				m_pData;
	uint32*				m_pReloc;

	uint32				m_nJccPos;
};

#endif
