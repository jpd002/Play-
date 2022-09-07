#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include "Types.h"

struct BIOS_DEBUG_MODULE_INFO
{
	std::string name;
	uint32 begin;
	uint32 end;
	void* param;
};

typedef std::vector<BIOS_DEBUG_MODULE_INFO> BiosDebugModuleInfoArray;
typedef BiosDebugModuleInfoArray::iterator BiosDebugModuleInfoIterator;

typedef std::variant<uint32, std::string> BIOS_DEBUG_OBJECT_FIELD;

struct BIOS_DEBUG_OBJECT
{
	std::vector<BIOS_DEBUG_OBJECT_FIELD> fields;
};
typedef std::vector<BIOS_DEBUG_OBJECT> BiosDebugObjectArray;

enum class BIOS_DEBUG_OBJECT_FIELD_TYPE
{
	UNKNOWN,
	UINT32,
	STRING,
};

enum class BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE
{
	NONE           = 0,
	IDENTIFIER     = (1 << 0),
	TEXT_ADDRESS   = (1 << 1),
	DATA_ADDRESS   = (1 << 2),
	HIDDEN         = (1 << 3),
	LOCATION       = (1 << 4),
	STACK_POINTER  = (1 << 5),
	RETURN_ADDRESS = (1 << 6),
};

static BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE operator |(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE lhs, BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE rhs)
{
	auto result = static_cast<uint32>(lhs) | (static_cast<uint32>(rhs));
	return static_cast<BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE>(result);
}

enum class BIOS_DEBUG_OBJECT_ACTION
{
	NONE,
	SHOW_LOCATION,
	SHOW_STACK_OR_LOCATION,
};

struct BIOS_DEBUG_OBJECT_FIELD_INFO
{
	std::string name;
	BIOS_DEBUG_OBJECT_FIELD_TYPE type = BIOS_DEBUG_OBJECT_FIELD_TYPE::UNKNOWN;
	BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE attributes = BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::NONE;
	
	bool HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE attr) const
	{
		return (static_cast<uint32>(attributes) & static_cast<uint32>(attr)) != 0;
	}
};
typedef std::vector<BIOS_DEBUG_OBJECT_FIELD_INFO> BiosDebugObjectFieldInfoArray;

struct BIOS_DEBUG_OBJECT_INFO
{
	std::string name;
	BiosDebugObjectFieldInfoArray fields;
	BIOS_DEBUG_OBJECT_ACTION selectionAction = BIOS_DEBUG_OBJECT_ACTION::NONE;
	
	int32 FindFieldWithAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE attribute)
	{
		for(int fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
		{
			const auto& field = fields[fieldIndex];
			if(field.HasAttribute(attribute))
			{
				return fieldIndex;
			}
		}
		return -1;
	}
};
typedef std::map<uint32, BIOS_DEBUG_OBJECT_INFO> BiosDebugObjectInfoMap;

class CBiosDebugInfoProvider
{
public:
	~CBiosDebugInfoProvider() = default;
#ifdef DEBUGGER_INCLUDED
	virtual BiosDebugModuleInfoArray GetModulesDebugInfo() const = 0;
	virtual BiosDebugObjectInfoMap GetBiosObjectsDebugInfo() const
	{
		return BiosDebugObjectInfoMap();
	}
	virtual BiosDebugObjectArray GetBiosObjects(uint32 typeId) const
	{
		return BiosDebugObjectArray();
	}
#endif
};
