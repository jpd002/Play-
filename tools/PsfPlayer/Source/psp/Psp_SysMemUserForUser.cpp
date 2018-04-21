#include "Psp_SysMemUserForUser.h"
#include "Log.h"

#define LOGNAME ("Psp_SysMemUserForUser")

using namespace Psp;

#define MEMORYTYPE_MASK (0x0F0000)
#define MEMORYTYPE_PARTITION (0x010000)
#define MEMORYTYPE_BLOCK (0x020000)

CSysMemUserForUser::CSysMemUserForUser(CBios& bios, uint8* ram)
    : m_bios(bios)
    , m_ram(ram)
{
}

CSysMemUserForUser::~CSysMemUserForUser()
{
}

std::string CSysMemUserForUser::GetName() const
{
	return "SysMemUserForUser";
}

uint32 CSysMemUserForUser::AllocPartitionMemory(uint32 partitionId, uint32 namePtr, uint32 type, uint32 size, uint32 addr)
{
	const char* name = NULL;
	if(namePtr != 0)
	{
		name = reinterpret_cast<char*>(m_ram + namePtr);
	}

#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "AllocPartitionMemory(partitionId = %d, name = '%s', type = %d, size = %d, addr = 0x%0.8X);\r\n",
	                          partitionId, name, type, size, addr);
#endif

	uint32 address = m_bios.Heap_AllocateMemory(size);
	uint32 result = m_bios.Heap_GetBlockId(address);
	assert(result != -1);
	assert((result & MEMORYTYPE_MASK) == 0);
	result |= MEMORYTYPE_PARTITION;

	return result;
}

uint32 CSysMemUserForUser::GetBlockHeadAddr(uint32 blockId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "GetBlockHeadAddr(blockId = 0x%0.8X);\r\n", blockId);
#endif

	uint32 blockType = blockId & MEMORYTYPE_MASK;
	assert(blockType == MEMORYTYPE_PARTITION);
	uint32 result = m_bios.Heap_GetBlockAddress(blockId & ~MEMORYTYPE_MASK);

	return result;
}

uint32 CSysMemUserForUser::AllocMemoryBlock(uint32 namePtr, uint32 flags, uint32 size, uint32 reserved)
{
	const char* name = NULL;
	if(namePtr != 0)
	{
		name = reinterpret_cast<char*>(m_ram + namePtr);
	}

#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "AllocMemoryBlock(namePtr = '%s', flags = 0x%0.8X, size = 0x%0.8X, reserved = 0x%0.8X);\r\n",
	                          name, flags, size, reserved);
#endif

	uint32 address = m_bios.Heap_AllocateMemory(size);
	uint32 result = m_bios.Heap_GetBlockId(address);
	assert(result != -1);
	assert((result & MEMORYTYPE_MASK) == 0);
	result |= MEMORYTYPE_BLOCK;

	return result;
}

uint32 CSysMemUserForUser::GetMemoryBlockAddr(uint32 blockId, uint32 ptrAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "GetMemoryBlockAddr(id = 0x%0.8X, ptrAddr = 0x%0.8X);\r\n",
	                          blockId, ptrAddr);
#endif

	assert(ptrAddr != 0);
	if(ptrAddr == 0)
	{
		return -1;
	}

	uint32 blockType = blockId & MEMORYTYPE_MASK;
	assert(blockType == MEMORYTYPE_BLOCK);
	if(blockType != MEMORYTYPE_BLOCK)
	{
		return -1;
	}

	uint32* ptr = reinterpret_cast<uint32*>(m_ram + ptrAddr);
	(*ptr) = m_bios.Heap_GetBlockAddress(blockId & ~MEMORYTYPE_MASK);

	uint32 result = 0;
	return result;
}

void CSysMemUserForUser::SetCompilerVersion(uint32 version)
{
	printf("Compiled using: gcc v%d.%d.%d\r\n", (version >> 16) & 0xFF, (version >> 8) & 0xFF, (version & 0xFF));
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetCompilerVersion(version = 0x%0.8X);\r\n", version);
#endif
}

void CSysMemUserForUser::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0x237DBD4F:
		context.m_State.nGPR[CMIPS::V0].nV0 = AllocPartitionMemory(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0,
		    context.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case 0x9D9A5BA1:
		context.m_State.nGPR[CMIPS::V0].nV0 = GetBlockHeadAddr(
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 0xFE707FDF:
		context.m_State.nGPR[CMIPS::V0].nV0 = AllocMemoryBlock(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 0xDB83A952:
		context.m_State.nGPR[CMIPS::V0].nV0 = GetMemoryBlockAddr(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 0xF77D77CB:
		SetCompilerVersion(context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X.\r\n", methodId);
		break;
	}
}
