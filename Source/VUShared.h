#ifndef _VUSHARED_H_
#define _VUSHARED_H_

#include "MIPSReflection.h"
#include "CacheBlock.h"
#include "uint128.h"

namespace VUShared
{
	enum VECTOR_COMP
	{
		VECTOR_COMPX = 0,
		VECTOR_COMPY = 1,
		VECTOR_COMPZ = 2,
		VECTOR_COMPW = 3,
	};

	int32				GetImm11Offset(uint16);
	int32				GetBranch(uint16);

	bool				DestinationHasElement(uint8, unsigned int);
	uint32*				GetVectorElement(CMIPS*, unsigned int, unsigned int);
	uint32*				GetAccumulatorElement(CMIPS*, unsigned int);

	void				PullVector(uint8, uint128*);

	//Shared instructions
	void				ABS(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				ADD(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				ADDbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8, uint8);
	void				ADDi(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				ADDq(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				ADDAbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				CLIP(CCacheBlock*, CMIPS*, uint8, uint8);
	void				DIV(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				FTOI0(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				FTOI4(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				ITOF0(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MADD(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				MADDbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8, uint8);
	void				MADDA(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MADDAbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				MADDAi(CCacheBlock*, CMIPS*, uint8, uint8);
	void				MAX(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				MAXbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8, uint8);
	void				MINI(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				MINIbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8, uint8);
	void				MINIi(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MOVE(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MR32(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MSUBi(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MSUBAi(CCacheBlock*, CMIPS*, uint8, uint8);
	void				MUL(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				MULbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8, uint8);
	void				MULi(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MULq(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MULA(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				MULAbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				MULAi(CCacheBlock*, CMIPS*, uint8, uint8);
	void				OPMSUB(CCacheBlock*, CMIPS*, uint8, uint8, uint8);
	void				OPMULA(CCacheBlock*, CMIPS*, uint8, uint8);
	void				RSQRT(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				SQRT(CCacheBlock*, CMIPS*, uint8, uint8);
	void				SUB(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8);
	void				SUBbc(CCacheBlock*, CMIPS*, uint8, uint8, uint8, uint8, uint8);
	void				SUBi(CCacheBlock*, CMIPS*, uint8, uint8, uint8);

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
	void				ReflOpQFtf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	void				ReflOpQFsfFtf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	extern const char*	m_sBroadcast[4];
	extern const char*	m_sDestination[16];
}

#endif
