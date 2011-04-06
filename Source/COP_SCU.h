#ifndef _COP_SCU_H_
#define _COP_SCU_H_

#include "MIPSCoprocessor.h"

class CCOP_SCU : public CMIPSCoprocessor
{
public:
	enum REGISTER
	{
        COUNT       = 0x09,
		STATUS		= 0x0C,
		EPC			= 0x0E,
		CPCOND0		= 0x15,
		ERROREPC	= 0x1E,
	};

							CCOP_SCU(MIPS_REGSIZE);
	virtual void			CompileInstruction(uint32, CMipsJitter*, CMIPS*);
	virtual void			GetInstruction(uint32, char*);
	virtual void			GetArguments(uint32, uint32, char*);
	virtual uint32			GetEffectiveAddress(uint32, uint32);
	virtual bool			IsBranch(uint32);

	static const char*		m_sRegName[];

private:
	typedef void (CCOP_SCU::*InstructionFuncConstant)();

	static InstructionFuncConstant	m_pOpGeneral[0x20];
    static InstructionFuncConstant  m_pOpBC0[0x20];
	static InstructionFuncConstant  m_pOpC0[0x40];

	uint8					m_nRT;
	uint8					m_nRD;

	//General
	void					MFC0();
	void					MTC0();
    void					BC0();
	void					C0();

    //BC0
    void					BC0F();
    void					BC0T();

	//C0
	void					ERET();
	void					EI();
	void					DI();
};

#endif
