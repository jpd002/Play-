#include "Ac3SoundStreamDecoder.h"

Ac3SoundStreamDecoder::Ac3SoundStreamDecoder()
{
}

Ac3SoundStreamDecoder::~Ac3SoundStreamDecoder()
{
}

void Ac3SoundStreamDecoder::Execute(Framework::CBitStream& stream)
{
	//Search syncWord
	uint16 syncWord = static_cast<uint16>(stream.PeekBits_MSBF(16));
	if(syncWord == 0x0B77)
	{
		stream.Advance(16);
		uint16 crc1 = static_cast<uint16>(stream.GetBits_MSBF(16));
		uint8 fscod = static_cast<uint8>(stream.GetBits_MSBF(2));
		uint8 frmsizecod = static_cast<uint8>(stream.GetBits_MSBF(6));

		ReadBsi(stream);
	}
}

void Ac3SoundStreamDecoder::ReadBsi(Framework::CBitStream& stream)
{
	uint8 bsid = static_cast<uint8>(stream.GetBits_MSBF(5));
	uint8 bsmod = static_cast<uint8>(stream.GetBits_MSBF(3));
	uint8 acmod = static_cast<uint8>(stream.GetBits_MSBF(3));
}
