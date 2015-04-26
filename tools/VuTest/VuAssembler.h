#pragma once

#include "Types.h"

class CVuAssembler
{
public:
	enum VF_REGISTER
	{
		VF0,  VF1,  VF2,  VF3,  VF4,  VF5,  VF6,  VF7,
		VF8,  VF9,  VF10, VF11, VF12, VF13, VF14, VF15,
		VF16, VF17, VF18, VF19, VF20, VF21, VF22, VF23,
		VF24, VF25, VF26, VF27, VF28, VF29, VF30, VF31
	};

	enum VI_REGISTER
	{
		VI0,  VI1,  VI2,  VI3,  VI4,  VI5,  VI6,  VI7,
		VI8,  VI9,  VI10, VI11, VI12, VI13, VI14, VI15,
	};

	enum DEST
	{
		DEST_NONE,
		DEST_W,
		DEST_Z,
		DEST_ZW,
		DEST_Y,
		DEST_YW,
		DEST_YZ,
		DEST_YZW,
		DEST_X,
		DEST_XW,
		DEST_XZ,
		DEST_XZW,
		DEST_XY,
		DEST_XYW,
		DEST_XYZ,
		DEST_XYZW,
	};

	enum BROADCAST
	{
		BC_X,
		BC_Y,
		BC_Z,
		BC_W,
	};

					CVuAssembler(uint32*);
	virtual			~CVuAssembler();

	void			Write(uint32, uint32);

	class Upper
	{
	public:
		enum
		{
			I_BIT = 0x80000000,
			E_BIT = 0x40000000,
		};

		static uint32		ADDi(DEST, VF_REGISTER, VF_REGISTER);
		static uint32		ITOF0(DEST, VF_REGISTER, VF_REGISTER);
		static uint32		MADDbc(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER, BROADCAST);
		static uint32		MADDAbc(DEST, VF_REGISTER, VF_REGISTER, BROADCAST);
		static uint32		MULi(DEST, VF_REGISTER, VF_REGISTER);
		static uint32		MULAbc(DEST, VF_REGISTER, VF_REGISTER, BROADCAST);
		static uint32		NOP();
		static uint32		OPMULA(VF_REGISTER, VF_REGISTER);
		static uint32		OPMSUB(VF_REGISTER, VF_REGISTER, VF_REGISTER);
		static uint32		SUBbc(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER, BROADCAST);
	};

	class Lower
	{
	public:
		static uint32		FMAND(VI_REGISTER, VI_REGISTER);
		static uint32		FSAND(VI_REGISTER, uint16);
		static uint32		NOP();
	};

private:
	uint32*			m_ptr = nullptr;
};
