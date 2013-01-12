#ifndef _AC3SOUNDSTREAMDECODER_H_
#define _AC3SOUNDSTREAMDECODER_H_

#include "BitStream.h"

class Ac3SoundStreamDecoder
{
public:
					Ac3SoundStreamDecoder();
	virtual			~Ac3SoundStreamDecoder();

	void			Execute(Framework::CBitStream&);

private:
	void			ReadBsi(Framework::CBitStream&);
};

#endif
