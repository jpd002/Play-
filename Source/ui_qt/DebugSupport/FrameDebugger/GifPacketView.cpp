#include "GifPacketView.h"
#include "gs/GSHandler.h"
#include "ee/GIF.h"
#include "uint128.h"
#include "string_format.h"

CGifPacketView::CGifPacketView(QWidget* parent)
    : QTextEdit(parent)
{
}

void CGifPacketView::SetPacket(const uint8* vuMem, uint32 packetAddress, uint32 packetSize)
{
	WriteInfoList writes;

	while(1)
	{
		CGIF::TAG tag;
		memcpy(&tag, vuMem + packetAddress, sizeof(tag));
		uint32 tagAddress = packetAddress;
		packetAddress += 0x10;

		uint32 tagSize = 0;
		switch(tag.cmd)
		{
		case 0:
			tagSize = tag.loops * tag.nreg * 0x10;
			break;
		case 1:
			tagSize = tag.loops * tag.nreg * 0x08;
			break;
		case 2:
			tagSize = tag.loops * 0x10;
			break;
		}

		if(tagSize == 0) break;
		if(tagSize > packetSize) break;

		if(tag.cmd == 0)
		{
			if(tag.pre)
			{
				writes.push_back(WriteInfo(tagAddress, GS_REG_PRIM, static_cast<uint64>(tag.prim)));
			}
		}

		packetSize -= tagSize;

		switch(tag.cmd)
		{
		case 0:
			DumpPacked(vuMem, packetAddress, tag, writes);
			break;
		default:
			assert(false);
			break;
		}
	}

	std::string result;
	for(unsigned int i = 0; i < writes.size(); i++)
	{
		const auto& writeInfo(writes[i]);
		result += string_format("%04X(%04X): %s\r\n", i, writeInfo.srcAddress,
		                        CGSHandler::DisassembleWrite(writeInfo.write.first, writeInfo.write.second).c_str());
	}

	setText(result.c_str());
}

void CGifPacketView::DumpPacked(const uint8* vuMem, uint32& packetAddress, const CGIF::TAG& tag, WriteInfoList& writes)
{
	uint64 qtemp = 0;
	WriteInfoList result;

	for(unsigned int i = 0; i < tag.loops; i++)
	{
		for(unsigned int j = 0; j < tag.nreg; j++)
		{
			uint32 regDesc = static_cast<uint32>((tag.regs >> (j * 4)) & 0x0F);

			uint128 input = *reinterpret_cast<const uint128*>(vuMem + packetAddress);
			uint32 writeAddress = packetAddress;
			packetAddress += 0x10;

			switch(regDesc)
			{
			case 0x00:
				//PRIM
				writes.push_back(WriteInfo(writeAddress, GS_REG_PRIM, input.nV0));
				break;
			case 0x01:
				//RGBA
				{
					uint64 temp = (input.nV[0] & 0xFF);
					temp |= (input.nV[1] & 0xFF) << 8;
					temp |= (input.nV[2] & 0xFF) << 16;
					temp |= (input.nV[3] & 0xFF) << 24;
					temp |= ((uint64)qtemp << 32);
					writes.push_back(WriteInfo(writeAddress, GS_REG_RGBAQ, temp));
				}
				break;
			case 0x02:
				//ST
				qtemp = input.nV2;
				writes.push_back(WriteInfo(writeAddress, GS_REG_ST, input.nD0));
				break;
			case 0x03:
				//UV
				{
					uint64 temp = (input.nV[0] & 0x7FFF);
					temp |= (input.nV[1] & 0x7FFF) << 16;
					writes.push_back(WriteInfo(writeAddress, GS_REG_UV, temp));
				}
				break;
			case 0x04:
				//XYZF2
				{
					uint64 temp = (input.nV[0] & 0xFFFF);
					temp |= (input.nV[1] & 0xFFFF) << 16;
					temp |= (uint64)(input.nV[2] & 0x0FFFFFF0) << 28;
					temp |= (uint64)(input.nV[3] & 0x00000FF0) << 52;
					writes.push_back(WriteInfo(writeAddress, ((input.nV[3] & 0x8000) != 0) ? GS_REG_XYZF3 : GS_REG_XYZF2, temp));
				}
				break;
			case 0x05:
				//XYZ2
				{
					uint64 temp = (input.nV[0] & 0xFFFF);
					temp |= (input.nV[1] & 0xFFFF) << 16;
					temp |= (uint64)(input.nV[2] & 0xFFFFFFFF) << 32;
					writes.push_back(WriteInfo(writeAddress, ((input.nV[3] & 0x8000) != 0) ? GS_REG_XYZ3 : GS_REG_XYZ2, temp));
				}
				break;
			case 0x06:
				//TEX0_1
				writes.push_back(WriteInfo(writeAddress, GS_REG_TEX0_1, input.nD0));
				break;
				/*
			case 0x07:
				//TEX0_2
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_TEX0_2, nPacket.nD0));
				break;
			case 0x08:
				//CLAMP_1
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_CLAMP_1, nPacket.nD0));
				break;
			case 0x0D:
				//XYZ3
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ3, nPacket.nD0));
				break;
*/
			case 0x0E:
				//A + D
				writes.push_back(WriteInfo(writeAddress, static_cast<uint8>(input.nD1), input.nD0));
				break;
			case 0x0F:
				//NOP
				break;
			default:
				assert(0);
				break;
			}
		}
	}
}
