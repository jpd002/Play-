#include <assert.h>
#include "ProgramStreamDecoder.h"

using namespace Framework;

ProgramStreamDecoder::ProgramStreamDecoder(Framework::CStream& inputStream)
: m_inputStream(inputStream)
, m_stream(inputStream)
{

}

ProgramStreamDecoder::~ProgramStreamDecoder()
{

}

void ProgramStreamDecoder::RegisterVideoStreamHandler(const VideoStreamHandler& handler)
{
	m_videoStreamHandler = handler;
}

void ProgramStreamDecoder::RegisterPrivateStream1Handler(const PrivateStreamHandler& handler)
{
	m_privateStream1Handler = handler;
}

ProgramStreamDecoder::STATUS ProgramStreamDecoder::Read()
{
	try
	{
		while(1)
		{
			uint32 marker = m_stream.PeekBits_MSBF(24);
			if(marker == 0x01)
			{
				m_stream.Advance(24);
				uint8 commandType = static_cast<uint8>(m_stream.GetBits_MSBF(8));
				
				switch(commandType)
				{
					case 0xBA:
						ReadProgramStreamPackHeader(m_stream);
						break;
					case 0xBB:
						ReadSystemHeader(m_stream);
						break;
					case 0xBD:
						if(!ReadPrivateStream1(m_stream))
						{
							return STATUS_INTERRUPTED;
						}
						break;
					case 0xBF:
						ReadPrivateStream2(m_stream);
						break;
					case 0xE0:
						if(!ReadVideoStream(m_stream))
						{
							return STATUS_INTERRUPTED;
						}
						break;
				}
			}
			else
			{
				m_stream.Advance(8);
			}
		}
	}
	catch(const Framework::CBitStream::CBitStreamException&)
	{
#ifdef _DEBUG
		printf("Exception occured while processing input stream. Bailing out.\r\n");
#endif
	}

	return STATUS_EOF;
}

void ProgramStreamDecoder::ReadAndValidateMarker(CBitStream& stream, unsigned int size, uint32 value)
{
	uint32 marker = stream.GetBits_MSBF(size);
	if(marker != value)
	{
		throw std::runtime_error("Invalid marker.");
	}
}

uint32 ProgramStreamDecoder::ReadPesExtension(CBitStream& stream)
{
	ReadAndValidateMarker(stream, 2, 0x02);
	uint32 scramblingControl		= stream.GetBits_MSBF(2);
	uint32 priority					= stream.GetBits_MSBF(1);
	uint32 dataAlignmentIndicator	= stream.GetBits_MSBF(1);
	uint32 copyright				= stream.GetBits_MSBF(1);
	uint32 original					= stream.GetBits_MSBF(1);
	uint32 ptsDtsFlags				= stream.GetBits_MSBF(2);
	uint32 escrFlag					= stream.GetBits_MSBF(1);
	uint32 esRateFlag				= stream.GetBits_MSBF(1);
	uint32 dsmTrickModeFlag			= stream.GetBits_MSBF(1);
	uint32 additionalCopyInfoFlag	= stream.GetBits_MSBF(1);
	uint32 crcFlag					= stream.GetBits_MSBF(1);
	uint32 extensionFlag			= stream.GetBits_MSBF(1);
	uint32 headerDataLength			= stream.GetBits_MSBF(8);
	//packetLength -= 3;

	uint32 calculatedDataLength = 0;
	if(ptsDtsFlags == 0)
	{
		//None
	}
	else if(ptsDtsFlags == 2)
	{
		//Read PTS
		ReadAndValidateMarker(stream, 4, 0x02);
		uint32 ptsTop = stream.GetBits_MSBF(3);
		ReadAndValidateMarker(stream, 1, 0x01);
		uint32 ptsMiddle = stream.GetBits_MSBF(15);
		ReadAndValidateMarker(stream, 1, 0x01);
		uint32 ptsBottom = stream.GetBits_MSBF(15);
		ReadAndValidateMarker(stream, 1, 0x01);
		calculatedDataLength += 5;
	}
	else if(ptsDtsFlags == 3)
	{
		//Read PTS
		ReadAndValidateMarker(stream, 4, 0x03);
		uint32 ptsTop = stream.GetBits_MSBF(3);
		ReadAndValidateMarker(stream, 1, 0x01);
		uint32 ptsMiddle = stream.GetBits_MSBF(15);
		ReadAndValidateMarker(stream, 1, 0x01);
		uint32 ptsBottom = stream.GetBits_MSBF(15);
		ReadAndValidateMarker(stream, 1, 0x01);
		calculatedDataLength += 5;

		//Read DTS
		ReadAndValidateMarker(stream, 4, 0x01);
		uint32 dtsTop = stream.GetBits_MSBF(3);
		ReadAndValidateMarker(stream, 1, 0x01);
		uint32 dtsMiddle = stream.GetBits_MSBF(15);
		ReadAndValidateMarker(stream, 1, 0x01);
		uint32 dtsBottom = stream.GetBits_MSBF(15);
		ReadAndValidateMarker(stream, 1, 0x01);
		calculatedDataLength += 5;
	}
	else
	{
		throw std::runtime_error("Invalid DTS/PTS flags");
	}

	assert(escrFlag == 0);
	assert(esRateFlag == 0);
	assert(dsmTrickModeFlag == 0);
	assert(additionalCopyInfoFlag == 0);
	assert(crcFlag == 0);

	if(extensionFlag)
	{
		uint32 privateDataFlag					= stream.GetBits_MSBF(1);
		uint32 packHeaderFieldFlag				= stream.GetBits_MSBF(1);
		uint32 programPacketSequenceCounterFlag = stream.GetBits_MSBF(1);
		uint32 pstdBufferFlag					= stream.GetBits_MSBF(1);
		ReadAndValidateMarker(stream, 3, 0x7);
		uint32 extensionFlag2					= stream.GetBits_MSBF(1);

		calculatedDataLength += 1;
		
		assert(privateDataFlag == 0);
		assert(packHeaderFieldFlag == 0);
		assert(programPacketSequenceCounterFlag == 0);

		if(pstdBufferFlag)
		{
			ReadAndValidateMarker(stream, 2, 0x01);
			uint32 bufferScale = stream.GetBits_MSBF(1);
			uint32 bufferSize = stream.GetBits_MSBF(13);
			
			calculatedDataLength += 2;
		}

		assert(extensionFlag2 == 0);
	}

	assert(calculatedDataLength <= headerDataLength);
	if(calculatedDataLength < headerDataLength)
	{
		uint32 stuffingLength = headerDataLength - calculatedDataLength;
		for(unsigned int i = 0; i < stuffingLength; i++)
		{
			stream.GetBits_MSBF(8);
		}
	}

	//+3 for the PES extension header
	return headerDataLength + 3;
}

void ProgramStreamDecoder::ReadProgramStreamPackHeader(CBitStream& stream)
{
	ReadAndValidateMarker(stream, 2, 0x01);
	uint32 scrTop = stream.GetBits_MSBF(3);
	ReadAndValidateMarker(stream, 1, 0x01);
	uint32 scrMiddle = stream.GetBits_MSBF(15);
	ReadAndValidateMarker(stream, 1, 0x01);
	uint32 scrBottom = stream.GetBits_MSBF(15);
	ReadAndValidateMarker(stream, 1, 0x01);
	uint32 scrExt = stream.GetBits_MSBF(9);
	ReadAndValidateMarker(stream, 1, 0x01);
	uint32 bitRate = stream.GetBits_MSBF(22);
	ReadAndValidateMarker(stream, 2, 0x03);
	uint32 reserved = stream.GetBits_MSBF(5);
	uint32 stuffingLength = stream.GetBits_MSBF(3);
	
	for(unsigned int i = 0; i < stuffingLength; i++)
	{
		uint8 stuff = static_cast<uint8>(stream.GetBits_MSBF(8));
	}
}

void ProgramStreamDecoder::ReadSystemHeader(CBitStream& stream)
{
	uint16 headerLength = static_cast<uint16>(stream.GetBits_MSBF(16));
	for(unsigned int i = 0; i < headerLength; i++)
	{
		uint8 stuff = static_cast<uint8>(stream.GetBits_MSBF(8));
	}
	//uint32 rateBound = stream.GetBits_MSBF(24);
	//uint32 audioBound = stream.GetBits_MSBF(8);
	//uint32 videoBound = stream.GetBits_MSBF(8);
	//uint32 packetRate = stream.GetBits_MSBF(8);
}

bool ProgramStreamDecoder::ReadPrivateStream1(CBitStream& stream)
{
	bool result = true;
	PRIVATE_STREAM1_INFO streamInfo;

	uint32 packetLength = static_cast<uint16>(stream.GetBits_MSBF(16));

	packetLength -= ReadPesExtension(stream);

	//Read private stream 1 stuff
	streamInfo.subStreamNumber		= static_cast<uint8>(stream.GetBits_MSBF(8));
	streamInfo.frameCount			= static_cast<uint8>(stream.GetBits_MSBF(8));
	streamInfo.firstAccessUnit		= static_cast<uint16>(stream.GetBits_MSBF(16));
	packetLength -= 4;

	if(m_privateStream1Handler)
	{
		result = m_privateStream1Handler(stream, streamInfo, packetLength);
	}
	else
	{
		for(unsigned int i = 0; i < packetLength; i++)
		{
			uint8 stuff = static_cast<uint8>(stream.GetBits_MSBF(8));
		}
	}
	return result;
}

void ProgramStreamDecoder::ReadPrivateStream2(CBitStream& stream)
{
	uint32 packetLength = static_cast<uint16>(stream.GetBits_MSBF(16));
	for(unsigned int i = 0; i < packetLength; i++)
	{
		uint8 stuff = static_cast<uint8>(stream.GetBits_MSBF(8));
	}
}

bool ProgramStreamDecoder::ReadVideoStream(CBitStream& stream)
{
	bool result = true;
	uint32 packetLength = static_cast<uint16>(stream.GetBits_MSBF(16));
	
	packetLength -= ReadPesExtension(stream);
	
	if(m_videoStreamHandler)
	{
		result = m_videoStreamHandler(stream, packetLength);
	}
	else
	{
		for(unsigned int i = 0; i < packetLength; i++)
		{
			stream.GetBits_MSBF(8);
		}
	}

	return result;
}
