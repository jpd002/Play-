#ifndef _PS2CONST_H_
#define _PS2CONST_H_

namespace PS2
{
    enum EERAMSIZE
    {
	    EERAMSIZE = 0x02000000,
    };

    enum IOP_RAM_SIZE
    {
        IOP_RAM_SIZE = 0x00200000,
    };

    enum IOP_SCRATCH_SIZE
    {
		IOP_SCRATCH_SIZE = 0x00000400,
    };

    enum
    {
        IOP_CLOCK_FREQ = (44100 * 256 * 3),
    };

    enum EEBIOSSIZE
    {
	    EEBIOSSIZE = 0x00400000,
    };

    enum SPRSIZE
    {
	    SPRSIZE = 0x00004000,
    };

    enum VUMEM0SIZE
    {
		VUMEM0ADDR = 0x11004000,
        VUMEM0SIZE = 0x00001000,
    };

    enum MICROMEM0SIZE
    {
        MICROMEM0SIZE = 0x00001000,
    };

    enum VUMEM1SIZE
    {
	    VUMEM1SIZE = 0x00004000,
    };

    enum MICROMEM1SIZE
    {
	    MICROMEM1SIZE = 0x00004000,
    };

    enum
    {
        SPU_RAM_SIZE = 0x00200000,
    };
}

#endif
