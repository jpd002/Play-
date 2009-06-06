#ifndef _VUSHARED_H_
#define _VUSHARED_H_

#include "MIPSReflection.h"
#include "CodeGen.h"
#include "uint128.h"
#include <string.h>

namespace VUShared
{
	enum VECTOR_COMP
	{
		VECTOR_COMPX = 0,
		VECTOR_COMPY = 1,
		VECTOR_COMPZ = 2,
		VECTOR_COMPW = 3,
	};

    struct PIPEINFO
    {
        size_t value;
        size_t heldValue;
        size_t target;
    };

	struct OPERANDSET
	{
		unsigned int writeF;
		unsigned int readF0;
		unsigned int readF1;
		unsigned int writeI;
		unsigned int readI0;
		unsigned int readI1;
	};

	struct VUINSTRUCTION;

	struct VUSUBTABLE
	{
		uint32			nShift;
		uint32			nMask;
		VUINSTRUCTION*	pTable;	
	};

	struct VUINSTRUCTION
	{
		const char*		name;
		VUSUBTABLE*		subTable;
		void			(*pGetAffectedOperands)(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32, uint32, OPERANDSET&);
	};

    int32				GetImm11Offset(uint16);
	int32				GetBranch(uint16);

	bool				DestinationHasElement(uint8, unsigned int);
	uint32*				GetVectorElement(CMIPS*, unsigned int, unsigned int);
	size_t				GetVectorElement(unsigned int, unsigned int);
	uint32*				GetAccumulatorElement(CMIPS*, unsigned int);
    size_t              GetAccumulatorElement(unsigned int);

    void                PullVector(CCodeGen*, uint8, size_t);

    void                ADDA_base(CCodeGen*, uint8, size_t, size_t, bool);
    void                MADD_base(CCodeGen*, uint8, size_t, size_t, size_t, bool);
    void                MSUB_base(CCodeGen*, uint8, size_t, size_t, size_t, bool);

	//Shared instructions
	void				ABS(CCodeGen*, uint8, uint8, uint8);
	void				ADD(CCodeGen*, uint8, uint8, uint8, uint8);
	void				ADDbc(CCodeGen*, uint8, uint8, uint8, uint8, uint8);
	void				ADDi(CCodeGen*, uint8, uint8, uint8);
	void				ADDq(CCodeGen*, uint8, uint8, uint8, uint32);
    void                ADDA(CCodeGen*, uint8, uint8, uint8);
	void				ADDAbc(CCodeGen*, uint8, uint8, uint8, uint8);
	void				CLIP(CCodeGen*, uint8, uint8);
	void				DIV(CCodeGen*, uint8, uint8, uint8, uint8, uint32, unsigned int);
	void				FTOI0(CCodeGen*, uint8, uint8, uint8);
	void				FTOI4(CCodeGen*, uint8, uint8, uint8);
    void                FTOI12(CCodeGen*, uint8, uint8, uint8);
	void				ITOF0(CCodeGen*, uint8, uint8, uint8);
    void                ITOF4(CCodeGen*, uint8, uint8, uint8);
    void                ITOF12(CCodeGen*, uint8, uint8, uint8);
    void                ITOF15(CCodeGen*, uint8, uint8, uint8);
	void				MADD(CCodeGen*, uint8, uint8, uint8, uint8);
	void				MADDbc(CCodeGen*, uint8, uint8, uint8, uint8, uint8);
    void                MADDq(CCodeGen*, uint8, uint8, uint8, uint32);
	void				MADDA(CCodeGen*, uint8, uint8, uint8);
	void				MADDAbc(CCodeGen*, uint8, uint8, uint8, uint8);
	void				MADDAi(CCodeGen*, uint8, uint8);
	void				MAX(CCodeGen*, uint8, uint8, uint8, uint8);
	void				MAXbc(CCodeGen*, uint8, uint8, uint8, uint8, uint8);
	void				MINI(CCodeGen*, uint8, uint8, uint8, uint8);
	void				MINIbc(CCodeGen*, uint8, uint8, uint8, uint8, uint8);
	void				MINIi(CCodeGen*, uint8, uint8, uint8);
	void				MOVE(CCodeGen*, uint8, uint8, uint8);
	void				MR32(CCodeGen*, uint8, uint8, uint8);
    void                MSUBbc(CCodeGen*, uint8, uint8, uint8, uint8, uint8);
	void				MSUBi(CCodeGen*, uint8, uint8, uint8);
    void                MSUBAbc(CCodeGen*, uint8, uint8, uint8, uint8);
	void				MSUBAi(CCodeGen*, uint8, uint8);
	void				MUL(CCodeGen*, uint8, uint8, uint8, uint8);
	void				MULbc(CCodeGen*, uint8, uint8, uint8, uint8, uint8);
	void				MULi(CCodeGen*, uint8, uint8, uint8);
	void				MULq(CCodeGen*, uint8, uint8, uint8, uint32);
	void				MULA(CCodeGen*, uint8, uint8, uint8);
	void				MULAbc(CCodeGen*, uint8, uint8, uint8, uint8);
	void				MULAi(CCodeGen*, uint8, uint8);
	void				OPMSUB(CCodeGen*, uint8, uint8, uint8);
	void				OPMULA(CCodeGen*, uint8, uint8);
	void				RINIT(CCodeGen*, uint8, uint8);
    void                RGET(CCodeGen*, uint8, uint8);
    void                RNEXT(CCodeGen*, uint8, uint8);
	void				RSQRT(CCodeGen*, uint8, uint8, uint8, uint8, uint32, unsigned int);
	void				RXOR(CCodeGen*, uint8, uint8);
	void				SQRT(CCodeGen*, uint8, uint8, uint32, unsigned int);
	void				SUB(CCodeGen*, uint8, uint8, uint8, uint8);
	void				SUBbc(CCodeGen*, uint8, uint8, uint8, uint8, uint8);
	void				SUBi(CCodeGen*, uint8, uint8, uint8);
    void                SUBAbc(CCodeGen*, uint8, uint8, uint8, uint8);
    void                WAITQ(CCodeGen*);

    void                FlushPipeline(const PIPEINFO&, CCodeGen*);
    void                QueueInPipeline(const PIPEINFO&, CCodeGen*, uint32);
    void                VerifyPipeline(const PIPEINFO&, CCodeGen*, uint32);

    //Shared addressing modes
	void				ReflOpFdFsI(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpFdFsQ(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpFdFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpFdFsFtBc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpFtFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpClip(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpAccFsI(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpAccFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpAccFsFtBc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpRFsf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpFtR(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpQFtf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpQFsfFtf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	void				ReflOpAffAccFsI(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffAccFsFt(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffAccFsFtBc(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffFdFsFt(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffFdFsFtBc(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffFdFsQ(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffFdFsI(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffRFsf(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffFtR(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void				ReflOpAffQFsfFtf(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);

	VUINSTRUCTION*		DereferenceInstruction(VUSUBTABLE*, uint32);
	void				SubTableAffectedOperands(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32, uint32, OPERANDSET&);

	extern const char*	m_sBroadcast[4];
	extern const char*	m_sDestination[16];
}

#endif
