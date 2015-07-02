#ifndef _VUSHARED_H_
#define _VUSHARED_H_

#include "../MIPSReflection.h"
#include "../MipsJitter.h"
#include "../uint128.h"
#include <string.h>

#undef ABS
#undef MAX

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
		size_t					value;
		size_t					heldValue;
		size_t					counter;
	};

	struct OPERANDSET
	{
		unsigned int			writeF;
		unsigned int			readF0;
		unsigned int			readF1;
		unsigned int			writeI;
		unsigned int			readI0;
		unsigned int			readI1;
		bool					syncQ;
		bool					readQ;
	};

	struct VUINSTRUCTION;

	struct VUSUBTABLE
	{
		uint32					nShift;
		uint32					nMask;
		VUINSTRUCTION*			pTable;
	};

	struct VUINSTRUCTION
	{
		const char*				name;
		VUSUBTABLE*				subTable;
		void					(*pGetAffectedOperands)(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32, uint32, OPERANDSET&);
	};

	int32						GetImm11Offset(uint16);
	int32						GetBranch(uint16);

	void						VerifyVuReflectionTable(MIPSReflection::INSTRUCTION*, VUShared::VUINSTRUCTION*, size_t);

	bool						DestinationHasElement(uint8, unsigned int);
	void						ComputeMemAccessAddr(CMipsJitter*, unsigned int, uint32, uint32);
	uint32						GetDestOffset(uint8);
	uint32*						GetVectorElement(CMIPS*, unsigned int, unsigned int);
	size_t						GetVectorElement(unsigned int, unsigned int);
	uint32*						GetAccumulatorElement(CMIPS*, unsigned int);
	size_t						GetAccumulatorElement(unsigned int);

	void						PullVector(CMipsJitter*, uint8, size_t);
	void						PushIntegerRegister(CMipsJitter*, unsigned int);

	void						ClampVector(CMipsJitter*);
	void						TestSZFlags(CMipsJitter*, uint8, size_t, uint32);

	void						ADDA_base(CMipsJitter*, uint8, size_t, size_t, bool);
	void						MADD_base(CMipsJitter*, uint8, size_t, size_t, size_t, bool, uint32);
	void						MADDA_base(CMipsJitter*, uint8, size_t, size_t, bool, uint32);
	void						SUBA_base(CMipsJitter*, uint8, size_t, size_t, bool);
	void						MSUB_base(CMipsJitter*, uint8, size_t, size_t, size_t, bool);
	void						MSUBA_base(CMipsJitter*, uint8, size_t, size_t, bool, uint32);

	//Shared instructions
	void						ABS(CMipsJitter*, uint8, uint8, uint8);
	void						ADD(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						ADDbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint8, uint32);
	void						ADDi(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						ADDq(CMipsJitter*, uint8, uint8, uint8);
	void						ADDA(CMipsJitter*, uint8, uint8, uint8);
	void						ADDAbc(CMipsJitter*, uint8, uint8, uint8, uint8);
	void						ADDAi(CMipsJitter*, uint8, uint8);
	void						CLIP(CMipsJitter*, uint8, uint8);
	void						DIV(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						FTOI0(CMipsJitter*, uint8, uint8, uint8);
	void						FTOI4(CMipsJitter*, uint8, uint8, uint8);
	void						FTOI12(CMipsJitter*, uint8, uint8, uint8);
	void						FTOI15(CMipsJitter*, uint8, uint8, uint8);
	void						IADD(CMipsJitter*, uint8, uint8, uint8);
	void						IADDI(CMipsJitter*, uint8, uint8, uint8);
	void						IAND(CMipsJitter*, uint8, uint8, uint8);
	void						ILWbase(CMipsJitter*, uint8);
	void						ILWR(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						IOR(CMipsJitter*, uint8, uint8, uint8);
	void						ISUB(CMipsJitter*, uint8, uint8, uint8);
	void						ITOF0(CMipsJitter*, uint8, uint8, uint8);
	void						ITOF4(CMipsJitter*, uint8, uint8, uint8);
	void						ITOF12(CMipsJitter*, uint8, uint8, uint8);
	void						ITOF15(CMipsJitter*, uint8, uint8, uint8);
	void						ISWbase(CMipsJitter*, uint8);
	void						ISWR(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						LQbase(CMipsJitter*, uint8, uint8);
	void						LQI(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						MADD(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						MADDbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint8, uint32);
	void						MADDi(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						MADDq(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						MADDA(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						MADDAbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						MADDAi(CMipsJitter*, uint8, uint8, uint32);
	void						MAX(CMipsJitter*, uint8, uint8, uint8, uint8);
	void						MAXbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint8);
	void						MAXi(CMipsJitter*, uint8, uint8, uint8);
	void						MINI(CMipsJitter*, uint8, uint8, uint8, uint8);
	void						MINIbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint8);
	void						MINIi(CMipsJitter*, uint8, uint8, uint8);
	void						MOVE(CMipsJitter*, uint8, uint8, uint8);
	void						MR32(CMipsJitter*, uint8, uint8, uint8);
	void						MSUB(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						MSUBbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint8);
	void						MSUBi(CMipsJitter*, uint8, uint8, uint8);
	void						MSUBq(CMipsJitter*, uint8, uint8, uint8);
	void						MSUBA(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						MSUBAbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						MSUBAi(CMipsJitter*, uint8, uint8, uint32);
	void						MFIR(CMipsJitter*, uint8, uint8, uint8);
	void						MTIR(CMipsJitter*, uint8, uint8, uint8);
	void						MUL(CMipsJitter*, uint8, uint8, uint8, uint8);
	void						MULbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint8, uint32);
	void						MULi(CMipsJitter*, uint8, uint8, uint8);
	void						MULq(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						MULA(CMipsJitter*, uint8, uint8, uint8);
	void						MULAbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						MULAi(CMipsJitter*, uint8, uint8);
	void						MULAq(CMipsJitter*, uint8, uint8);
	void						OPMSUB(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						OPMULA(CMipsJitter*, uint8, uint8);
	void						RINIT(CMipsJitter*, uint8, uint8);
	void						RGET(CMipsJitter*, uint8, uint8);
	void						RNEXT(CMipsJitter*, uint8, uint8);
	void						RSQRT(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						RXOR(CMipsJitter*, uint8, uint8);
	void						SQbase(CMipsJitter*, uint8, uint8);
	void						SQD(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						SQI(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						SQRT(CMipsJitter*, uint8, uint8, uint32);
	void						SUB(CMipsJitter*, uint8, uint8, uint8, uint8, uint32);
	void						SUBbc(CMipsJitter*, uint8, uint8, uint8, uint8, uint8, uint32);
	void						SUBi(CMipsJitter*, uint8, uint8, uint8, uint32);
	void						SUBq(CMipsJitter*, uint8, uint8, uint8);
	void						SUBA(CMipsJitter*, uint8, uint8, uint8);
	void						SUBAbc(CMipsJitter*, uint8, uint8, uint8, uint8);
	void						SUBAi(CMipsJitter*, uint8, uint8);
	void						WAITQ(CMipsJitter*);

	void						FlushPipeline(const PIPEINFO&, CMipsJitter*);
	void						CheckPipeline(const PIPEINFO&, CMipsJitter*, uint32);
	void						QueueInPipeline(const PIPEINFO&, CMipsJitter*, uint32, uint32);
	void						CheckMacFlagPipeline(CMipsJitter*, uint32);

	//Shared addressing modes
	void						ReflOpFdFsI(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFdFsQ(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFdFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFdFsFtBc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFsDstItDec(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFsDstItInc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFtFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFtIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFtDstIsInc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpClip(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpAccFsI(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpAccFsQ(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpAccFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpAccFsFtBc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpRFsf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpFtR(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpQFtf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpQFsfFtf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpIdIsIt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpItFsf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpItIsDst(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void						ReflOpItIsImm5(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	void						ReflOpAffNone(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffAccFsI(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffFdFsQ(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffFdFsI(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffRFsf(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffFtR(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffQ(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	
	void						ReflOpAffWrARdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffWrARdFsQ(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffWrCfRdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffWrFdRdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffWrQRdFt(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);
	void						ReflOpAffWrQRdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32, OPERANDSET&);

	VUINSTRUCTION*				DereferenceInstruction(VUSUBTABLE*, uint32);
	void						SubTableAffectedOperands(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32, uint32, OPERANDSET&);

	extern const char*			m_sBroadcast[4];
	extern const char*			m_sDestination[16];
	extern const PIPEINFO		g_pipeInfoQ;
}

#endif
