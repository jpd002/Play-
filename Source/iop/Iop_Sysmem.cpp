#include "Iop_Sysmem.h"
#include "../Log.h"
#include "Iop_SifMan.h"

using namespace Iop;

#define LOG_NAME ("iop_sysmem")

#define FUNCTION_ALLOCATEMEMORY		"AllocateMemory"
#define FUNCTION_FREEMEMORY			"FreeMemory"
#define FUNCTION_PRINTF				"printf"

#define MIN_BLOCK_SIZE  0x20

CSysmem::CSysmem(uint8* ram, uint32 memoryBegin, uint32 memoryEnd, uint32 blockBase, CStdio& stdio, CIoman& ioman, CSifMan& sifMan)
: m_iopRam(ram)
, m_memoryBegin(memoryBegin)
, m_memoryEnd(memoryEnd)
, m_stdio(stdio)
, m_ioman(ioman)
, m_memorySize(memoryEnd - memoryBegin)
, m_blocks(reinterpret_cast<BLOCK*>(&ram[blockBase]), 1, MAX_BLOCKS)
{
	//Initialize block map
	m_headBlockId = m_blocks.Allocate();
	BLOCK* block = m_blocks[m_headBlockId];
	block->address		= m_memorySize;
	block->size			= 0;
	block->nextBlock	= 0;

	//Register sif module
	sifMan.RegisterModule(MODULE_ID, this);
}

CSysmem::~CSysmem()
{

}

std::string CSysmem::GetId() const
{
	return "sysmem";
}

std::string CSysmem::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_ALLOCATEMEMORY;
		break;
	case 5:
		return FUNCTION_FREEMEMORY;
		break;
	case 14:
		return FUNCTION_PRINTF;
		break;
	default:
		return "unknown";
		break;
	}
}

void CSysmem::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(AllocateMemory(
			context.m_State.nGPR[CMIPS::A1].nV[0],
			context.m_State.nGPR[CMIPS::A0].nV[0],
			context.m_State.nGPR[CMIPS::A2].nV[0]
			));
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(FreeMemory(
			context.m_State.nGPR[CMIPS::A0].nV[0]
			));
		break;
	case 14:
		m_stdio.__printf(context);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
		break;
	}
}

bool CSysmem::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
		assert(retSize == 4);
		ret[0] = SifAllocate(args[0]);
		break;
	case 0x02:
		assert(argsSize == 4);
		ret[0] = SifFreeMemory(args[0]);
		break;
	case 0x03:
		assert(argsSize >= 4);
		assert(retSize == 4);
		ret[0] = SifLoadMemory(args[0], reinterpret_cast<const char*>(args) + 4);
		break;
	case 0x04:
		assert(retSize == 4);
		ret[0] = SifAllocateSystemMemory(args[0], args[1], args[2]);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
	return true;
}

uint32 CSysmem::AllocateMemory(uint32 size, uint32 flags, uint32 wantedAddress)
{
	CLog::GetInstance().Print(LOG_NAME, "AllocateMemory(size = 0x%0.8X, flags = 0x%0.8X, wantedAddress = 0x%0.8X);\r\n",
		size, flags, wantedAddress);

	const uint32 blockSize = MIN_BLOCK_SIZE;
	size = ((size + (blockSize - 1)) / blockSize) * blockSize;

	if(flags == 0 || flags == 1)
	{
		uint32 begin = 0;
		uint32* nextBlockId = &m_headBlockId;
		BLOCK* nextBlock = m_blocks[*nextBlockId];
		while(nextBlock != NULL)
		{
			uint32 end = nextBlock->address;
			if((end - begin) >= size)
			{
				break;
			}
			begin = nextBlock->address + nextBlock->size;
			nextBlockId = &nextBlock->nextBlock;
			nextBlock = m_blocks[*nextBlockId];
		}
		
		if(nextBlock != NULL)
		{
			uint32 newBlockId = m_blocks.Allocate();
			assert(newBlockId != 0);
			if(newBlockId == 0)
			{
				return 0;
			}
			BLOCK* newBlock = m_blocks[newBlockId];
			newBlock->address	= begin;
			newBlock->size		= size;
			newBlock->nextBlock	= *nextBlockId;
			*nextBlockId = newBlockId;
			return begin + m_memoryBegin;
		}
	}
	else if(flags == 2)
	{
		assert(wantedAddress != 0);
		wantedAddress -= m_memoryBegin;

		uint32 begin = 0;
		uint32* nextBlockId = &m_headBlockId;
		BLOCK* nextBlock = m_blocks[*nextBlockId];
		while(nextBlock != NULL)
		{
			uint32 end = nextBlock->address;
			if(begin > wantedAddress)
			{
				//Couldn't find suitable space
				nextBlock = NULL;
				break;
			}
			if(
				(begin <= wantedAddress) && 
				((end - begin) >= size)
				)
			{
				break;
			}
			begin = nextBlock->address + nextBlock->size;
			nextBlockId = &nextBlock->nextBlock;
			nextBlock = m_blocks[*nextBlockId];
		}
		
		if(nextBlock != NULL)
		{
			uint32 newBlockId = m_blocks.Allocate();
			assert(newBlockId != 0);
			if(newBlockId == 0)
			{
				return 0;
			}
			BLOCK* newBlock = m_blocks[newBlockId];
			newBlock->address	= wantedAddress;
			newBlock->size		= size;
			newBlock->nextBlock	= *nextBlockId;
			*nextBlockId = newBlockId;
			return wantedAddress + m_memoryBegin;
		}
	}
	else
	{
		assert(0);
	}

	assert(0);
	return NULL;
}

uint32 CSysmem::FreeMemory(uint32 address)
{
	CLog::GetInstance().Print(LOG_NAME, "FreeMemory(address = 0x%0.8X);\r\n", address);

	address -= m_memoryBegin;
	//Search for block pointing at the address
	uint32* nextBlockId = &m_headBlockId;
	BLOCK* nextBlock = m_blocks[*nextBlockId];
	while(nextBlock != NULL)
	{
		if(nextBlock->address == address)
		{
			break;
		}
		nextBlockId = &nextBlock->nextBlock;
		nextBlock = m_blocks[*nextBlockId];
	}

	if(nextBlock != NULL)
	{
		m_blocks.Free(*nextBlockId);
		*nextBlockId = nextBlock->nextBlock;
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "%s: Trying to unallocate an unexisting memory block (0x%0.8X).\r\n", __FUNCTION__, address);
	}
	return 0;
}

uint32 CSysmem::SifAllocate(uint32 nSize)
{
	uint32 result = AllocateMemory(nSize, 0, 0); 
	CLog::GetInstance().Print(LOG_NAME, "result = 0x%0.8X = Allocate(size = 0x%0.8X);\r\n", 
		result, nSize);
	return result;
}

uint32 CSysmem::SifAllocateSystemMemory(uint32 nSize, uint32 nFlags, uint32 nPtr)
{
	uint32 result = AllocateMemory(nSize, nFlags, nPtr);
	CLog::GetInstance().Print(LOG_NAME, "result = 0x%0.8X = AllocateSystemMemory(flags = 0x%0.8X, size = 0x%0.8X, ptr = 0x%0.8X);\r\n", 
		result, nFlags, nSize, nPtr);
	return result;
}

uint32 CSysmem::SifLoadMemory(uint32 address, const char* filePath)
{
	CLog::GetInstance().Print(LOG_NAME, "LoadMemory(address = 0x%0.8X, filePath = '%s');\r\n",
		address, filePath);

	auto fd = m_ioman.Open(Ioman::CDevice::OPEN_FLAG_RDONLY, filePath);
	if(static_cast<int32>(fd) < 0)
	{
		return fd;
	}
	uint32 fileSize = m_ioman.Seek(fd, 0, CIoman::SEEK_DIR_END);
	m_ioman.Seek(fd, 0, CIoman::SEEK_DIR_SET);
	m_ioman.Read(fd, fileSize, m_iopRam + address);
	m_ioman.Close(fd);
	return 0;
}

uint32 CSysmem::SifFreeMemory(uint32 address)
{
	CLog::GetInstance().Print(LOG_NAME, "FreeMemory(address = 0x%0.8X);\r\n", address);
	FreeMemory(address);
	return 0;
}
