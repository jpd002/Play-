#ifndef _MEMORYMAP_H_
#define _MEMORYMAP_H_

#include "Types.h"
#include <functional>
#include <vector>

enum MEMORYMAP_ENDIANESS
{
	MEMORYMAP_ENDIAN_LSBF,
	MEMORYMAP_ENDIAN_MSBF,
};

class CMemoryMap
{
public:
	typedef std::function<uint32 (uint32, uint32)> MemoryMapHandlerType;

	enum MEMORYMAP_TYPE
	{
		MEMORYMAP_TYPE_MEMORY,
		MEMORYMAP_TYPE_FUNCTION
	};

	struct MEMORYMAPELEMENT
	{
		uint32								nStart;
		uint32								nEnd;
		void*								pPointer;
		MemoryMapHandlerType				handler;
		MEMORYMAP_TYPE						nType;
	};

	virtual									~CMemoryMap();
	uint8									GetByte(uint32);
	virtual uint16							GetHalf(uint32) = 0;
	virtual uint32							GetWord(uint32) = 0;
	virtual uint32							GetInstruction(uint32) = 0;
	virtual void							SetByte(uint32, uint8);
	virtual void							SetHalf(uint32, uint16) = 0;
	virtual void							SetWord(uint32, uint32) = 0;
	void									InsertReadMap(uint32, uint32, void*, unsigned char);
	void									InsertReadMap(uint32, uint32, const MemoryMapHandlerType&, unsigned char);
	void									InsertWriteMap(uint32, uint32, void*, unsigned char);
	void									InsertWriteMap(uint32, uint32, const MemoryMapHandlerType&, unsigned char);
	void									InsertInstructionMap(uint32, uint32, void*, unsigned char);
	const MEMORYMAPELEMENT*					GetReadMap(uint32) const;
	const MEMORYMAPELEMENT*					GetWriteMap(uint32) const;

protected:
	typedef std::vector<MEMORYMAPELEMENT> MemoryMapListType;

	static const MEMORYMAPELEMENT*			GetMap(const MemoryMapListType&, uint32);

	MemoryMapListType						m_instructionMap;
	MemoryMapListType						m_readMap;
	MemoryMapListType						m_writeMap;

private:
	static void								InsertMap(MemoryMapListType&, uint32, uint32, void*, unsigned char);
	static void								InsertMap(MemoryMapListType&, uint32, uint32, const MemoryMapHandlerType&, unsigned char);
};

class CMemoryMap_LSBF : public CMemoryMap
{
public:
	uint16									GetHalf(uint32);
	uint32									GetWord(uint32);
	uint32									GetInstruction(uint32);
	void									SetHalf(uint32, uint16);
	void									SetWord(uint32, uint32);
};

#endif
