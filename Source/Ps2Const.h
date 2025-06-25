#ifndef _PS2CONST_H_
#define _PS2CONST_H_

namespace PS2
{
	enum
	{
		EE_RAM_SIZE = 0x04000000,
		EE_BASE_RAM_SIZE = 0x02000000,
		EE_EXT_RAM_SIZE = 0x04000000,
	};

	enum
	{
		EE_CLOCK_FREQ = 0x11940000 / 4
	};

	enum
	{
		EE_BIOS_ADDR = 0x1FC00000,
		EE_BIOS_SIZE = 0x00400000,
	};

	enum
	{
		//Technically, SPR isn't mapped in the EE's physical address space,
		//but we map it after RAM for convenience.
		EE_SPR_ADDR = EE_RAM_SIZE,
		EE_SPR_SIZE = 0x00004000,
	};

	enum
	{
		GS_NTSC_HSYNC_FREQ = 15734,
		GS_PAL_HSYNC_FREQ = 15625
	};

	enum
	{
		GPU_DOT_CLOCK_FREQ = 13305600 //Ref: NO$PSX documentation
	};

	enum
	{
		IOP_RAM_SIZE = 0x00400000,
		IOP_BASE_RAM_SIZE = 0x00200000,
		IOP_EXT_RAM_SIZE = 0x00400000,
	};

	enum
	{
		IOP_SCRATCH_ADDR = 0x1F800000,
		IOP_SCRATCH_SIZE = 0x00001000
	};

	enum
	{
		IOP_CLOCK_BASE_FREQ = (44100 * 256 * 3),
		IOP_CLOCK_OVER_FREQ = (48000 * 256 * 3)
	};

	enum
	{
		VUMEM0ADDR = 0x11004000,
		VUMEM0SIZE = 0x00001000,
	};

	enum
	{
		MICROMEM0ADDR = 0x11000000,
		MICROMEM0SIZE = 0x00001000,
	};

	enum
	{
		VUMEM1ADDR = 0x1100C000,
		VUMEM1SIZE = 0x00004000,
	};

	enum
	{
		MICROMEM1ADDR = 0x11008000,
		MICROMEM1SIZE = 0x00004000,
	};

	enum
	{
		SPU_RAM_SIZE = 0x00200000,
	};
}

#endif
