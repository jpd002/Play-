#ifndef _PS2CONST_H_
#define _PS2CONST_H_

namespace PS2
{
    enum EERAMSIZE
    {
	    EERAMSIZE = 0x02000000,
    };

    enum IOPRAMSIZE
    {
        IOPRAMSIZE = 0x00400000,
    };

    enum
    {
        IOP_CLOCK_FREQUENCY = (44100 * 256 * 3),
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
}

#endif
