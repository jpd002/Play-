#pragma once

#include <cstddef>
#include <map>

#include "Types.h"

class CVuAssembler
{
public:
	enum VF_REGISTER
	{
		VF0,
		VF1,
		VF2,
		VF3,
		VF4,
		VF5,
		VF6,
		VF7,
		VF8,
		VF9,
		VF10,
		VF11,
		VF12,
		VF13,
		VF14,
		VF15,
		VF16,
		VF17,
		VF18,
		VF19,
		VF20,
		VF21,
		VF22,
		VF23,
		VF24,
		VF25,
		VF26,
		VF27,
		VF28,
		VF29,
		VF30,
		VF31
	};

	enum VI_REGISTER
	{
		VI0,
		VI1,
		VI2,
		VI3,
		VI4,
		VI5,
		VI6,
		VI7,
		VI8,
		VI9,
		VI10,
		VI11,
		VI12,
		VI13,
		VI14,
		VI15,
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

	enum FVF
	{
		FVF_X,
		FVF_Y,
		FVF_Z,
		FVF_W
	};

	enum
	{
		INSTRUCTION_SIZE = 8,
	};

	CVuAssembler(uint32*);
	virtual ~CVuAssembler();

	typedef uint32 LABEL;

	struct BRANCHOP
	{
		uint32 op = 0;
		LABEL label = 0;
	};

	unsigned int GetProgramSize() const;
	LABEL CreateLabel();
	void MarkLabel(LABEL);

	void Write(uint32, uint32);
	void Write(uint32, BRANCHOP);

	class Upper
	{
	public:
		enum
		{
			I_BIT = 0x80000000,
			E_BIT = 0x40000000,
		};

		static uint32 ADDbc(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER, BROADCAST);
		static uint32 ADDi(DEST, VF_REGISTER, VF_REGISTER);
		static uint32 CLIP(VF_REGISTER, VF_REGISTER);
		static uint32 FTOI4(DEST, VF_REGISTER, VF_REGISTER);
		static uint32 ITOF0(DEST, VF_REGISTER, VF_REGISTER);
		static uint32 ITOF12(DEST, VF_REGISTER, VF_REGISTER);
		static uint32 MADDbc(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER, BROADCAST);
		static uint32 MADDAbc(DEST, VF_REGISTER, VF_REGISTER, BROADCAST);
		static uint32 MULi(DEST, VF_REGISTER, VF_REGISTER);
		static uint32 MULq(DEST, VF_REGISTER, VF_REGISTER);
		static uint32 MULAbc(DEST, VF_REGISTER, VF_REGISTER, BROADCAST);
		static uint32 MAX(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER);
		static uint32 MINI(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER);
		static uint32 NOP();
		static uint32 OPMULA(VF_REGISTER, VF_REGISTER);
		static uint32 OPMSUB(VF_REGISTER, VF_REGISTER, VF_REGISTER);
		static uint32 SUB(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER);
		static uint32 SUBbc(DEST, VF_REGISTER, VF_REGISTER, VF_REGISTER, BROADCAST);
	};

	class Lower
	{
	public:
		static BRANCHOP B(LABEL);
		static uint32 DIV(VF_REGISTER, FVF, VF_REGISTER, FVF);
		static uint32 FCAND(uint32);
		static uint32 FCGET(VI_REGISTER);
		static uint32 FMAND(VI_REGISTER, VI_REGISTER);
		static uint32 FSAND(VI_REGISTER, uint16);
		static uint32 IADDIU(VI_REGISTER, VI_REGISTER, uint16);
		static BRANCHOP IBEQ(VI_REGISTER, VI_REGISTER, LABEL);
		static uint32 LQ(DEST, VF_REGISTER, uint16, VI_REGISTER);
		static uint32 MFIR(DEST, VF_REGISTER, VI_REGISTER);
		static uint32 MTIR(VI_REGISTER, VF_REGISTER, FVF);
		static uint32 NOP();
		static uint32 SQ(DEST, VF_REGISTER, uint16, VI_REGISTER);
		static uint32 WAITQ();
	};

private:
	void ResolveLabelReferences();
	void CreateLabelReference(LABEL);

	struct LABELREF
	{
		size_t address;
	};

	typedef std::map<LABEL, size_t> LabelMapType;
	typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;

	uint32* m_ptr = nullptr;
	uint32* m_startPtr = nullptr;
	LabelMapType m_labels;
	LabelReferenceMapType m_labelReferences;
	uint32 m_nextLabelId = 1;
};
