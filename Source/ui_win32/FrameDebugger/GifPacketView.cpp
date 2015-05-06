#include "GifPacketView.h"
#include "win32/DeviceContext.h"
#include "../../gs/GSHandler.h"
#include "../../ee/GIF.h"
#include "../../uint128.h"
#include "string_format.h"

CGifPacketView::CGifPacketView(HWND parentWnd, const RECT& rect)
: CRegViewPage(parentWnd, rect)
{

}

CGifPacketView::~CGifPacketView()
{

}

std::string DumpPacked(uint8*& packet, const CGIF::TAG& tag, CGSHandler::RegisterWriteList& registerWrites)
{
	uint64 qtemp = 0;
	std::string result;

	for(unsigned int i = 0; i < tag.loops; i++)
	{
		for(unsigned int j = 0; j < tag.nreg; j++)
		{
			uint32 regDesc = static_cast<uint32>((tag.regs >> (j * 4)) & 0x0F);

			uint128 input = *reinterpret_cast<uint128*>(packet);
			packet += 0x10;

			switch(regDesc)
			{
			case 0x00:
				//PRIM
				registerWrites.push_back(CGSHandler::RegisterWrite(GS_REG_PRIM, input.nV0));
				break;
			case 0x01:
				//RGBA
				{
					uint64 temp	 = (input.nV[0] & 0xFF);
					temp		|= (input.nV[1] & 0xFF) << 8;
					temp		|= (input.nV[2] & 0xFF) << 16;
					temp		|= (input.nV[3] & 0xFF) << 24;
					temp		|= ((uint64)qtemp << 32);
					registerWrites.push_back(CGSHandler::RegisterWrite(GS_REG_RGBAQ, temp));
				}
				break;
			case 0x02:
				//ST
				qtemp = input.nV2;
				registerWrites.push_back(CGSHandler::RegisterWrite(GS_REG_ST, input.nD0));
				break;
			case 0x03:
				//UV
				{
					uint64 temp	 = (input.nV[0] & 0x7FFF);
					temp		|= (input.nV[1] & 0x7FFF) << 16;
					registerWrites.push_back(CGSHandler::RegisterWrite(GS_REG_UV, temp));
				}
				break;
			case 0x04:
				//XYZF2
				{
					uint64 temp	 = (input.nV[0] & 0xFFFF);
					temp		|= (input.nV[1] & 0xFFFF) << 16;
					temp		|= (uint64)(input.nV[2] & 0x0FFFFFF0) << 28;
					temp		|= (uint64)(input.nV[3] & 0x00000FF0) << 52;
					registerWrites.push_back(CGSHandler::RegisterWrite(((input.nV[3] & 0x8000) != 0) ? GS_REG_XYZF3 : GS_REG_XYZF2, temp));
				}
				break;
/*
			case 0x05:
				//XYZ2
				nTemp  = (nPacket.nV[0] & 0xFFFF);
				nTemp |= (nPacket.nV[1] & 0xFFFF) << 16;
				nTemp |= (uint64)(nPacket.nV[2] & 0xFFFFFFFF) << 32;
				if(nPacket.nV[3] & 0x8000)
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ3, nTemp));
				}
				else
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ2, nTemp));
				}
				break;
			case 0x06:
				//TEX0_1
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_TEX0_1, nPacket.nD0));
				break;
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
				registerWrites.push_back(CGSHandler::RegisterWrite(static_cast<uint8>(input.nD1), input.nD0));
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

	return result;
}

void CGifPacketView::SetPacket(uint8* packet, uint32 packetSize)
{
	CGSHandler::RegisterWriteList registerWrites;

	while(1)
	{
		CGIF::TAG tag;
		memcpy(&tag, packet, sizeof(tag));
		packet += 0x10;

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
				registerWrites.push_back(CGSHandler::RegisterWrite(GS_REG_PRIM, tag.prim));
			}
		}

		packetSize -= tagSize;

		switch(tag.cmd)
		{
		case 0:
			DumpPacked(packet, tag, registerWrites);
			break;
		}
	}

	std::string result;
	for(unsigned int i = 0; i < registerWrites.size(); i++)
	{
		const auto& registerWrite(registerWrites[i]);
		result += string_format("%0.4X: %s\r\n", i, CGSHandler::DisassembleWrite(registerWrite.first, registerWrite.second).c_str());
	}

	SetDisplayText(result.c_str());
}
