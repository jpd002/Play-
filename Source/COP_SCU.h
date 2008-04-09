#ifndef _COP_SCU_H_
#define _COP_SCU_H_

#include "MIPSCoprocessor.h"

class CCOP_SCU : public CMIPSCoprocessor
{
public:
	enum REGISTER
	{
		STATUS		= 0x0C,
		EPC			= 0x0E,
		ERROREPC	= 0x1E,
	};

						CCOP_SCU(MIPS_REGSIZE);
	virtual void		CompileInstruction(uint32, CCodeGen*, CMIPS*, bool);
	virtual void		GetInstruction(uint32, char*);
	virtual void		GetArguments(uint32, uint32, char*);
	virtual uint32		GetEffectiveAddress(uint32, uint32);
	virtual bool		IsBranch(uint32);

	static char*		m_sRegName[];

private:

	static void			(*m_pOpGeneral[0x20])();
	static void			(*m_pOpCO[0x40])();

	static uint8		m_nRT;
	static uint8		m_nRD;

	//General
	static void			MFC0();
	static void			MTC0();
	static void			CO();

	//CO
	static void			ERET();
	static void			EI();
	static void			DI();
};

extern CCOP_SCU g_COPSCU;

#endif
