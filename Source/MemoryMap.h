#ifndef _MEMORYMAP_H_
#define _MEMORYMAP_H_

#include "List.h"
#include "Types.h"
#include <boost/function.hpp>

enum MEMORYMAP_TYPE
{
	MEMORYMAP_TYPE_MEMORY,
	MEMORYMAP_TYPE_FUNCTION
};

enum MEMORYMAP_ENDIANESS
{
	MEMORYMAP_ENDIAN_LSBF,
	MEMORYMAP_ENDIAN_MSBF,
};

struct MEMORYMAPELEMENT
{
	uint32			nStart;
	uint32			nEnd;
	void*			pPointer;
	MEMORYMAP_TYPE	nType;
};

class CMemoryMap
{
public:
	typedef boost::function<void (uint32)> WriteNotifyHandlerType;

	virtual									~CMemoryMap();
	uint8									GetByte(uint32);
	virtual uint16							GetHalf(uint32) = 0;
	virtual uint32							GetWord(uint32) = 0;
	virtual void							SetByte(uint32, uint8);
	virtual void							SetHalf(uint32, uint16) = 0;
	virtual void							SetWord(uint32, uint32) = 0;
	void									InsertReadMap(uint32, uint32, void*, MEMORYMAP_TYPE, unsigned char);
	void									InsertWriteMap(uint32, uint32, void*, MEMORYMAP_TYPE, unsigned char);
	void									SetWriteNotifyHandler(WriteNotifyHandlerType);

protected:
	WriteNotifyHandlerType					m_WriteNotifyHandler;
	static MEMORYMAPELEMENT*				GetMap(Framework::CList<MEMORYMAPELEMENT>*, uint32);
	Framework::CList<MEMORYMAPELEMENT>		m_Read;
	Framework::CList<MEMORYMAPELEMENT>		m_Write;

private:
	static void								InsertMap(Framework::CList<MEMORYMAPELEMENT>*, uint32, uint32, void*, MEMORYMAP_TYPE, unsigned char);
	static void								DeleteMap(Framework::CList<MEMORYMAPELEMENT>*);
};

class CMemoryMap_LSBF : public CMemoryMap
{
public:
	uint16									GetHalf(uint32);
	uint32									GetWord(uint32);
	void									SetHalf(uint32, uint16);
	void									SetWord(uint32, uint32);
};

#endif
