#include "Iop_Sysclib.h"

using namespace Iop;

CSysclib::CSysclib(uint8* ram, CStdio& stdio)
: m_ram(ram)
, m_stdio(stdio)
{

}

CSysclib::~CSysclib()
{

}

std::string CSysclib::GetId() const
{
	return "sysclib";
}

std::string CSysclib::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 6:
		return "toupper";
		break;
	case 7:
		return "tolower";
		break;
	case 8:
		return "look_ctype_table";
		break;
	case 11:
		return "memcmp";
		break;
	case 12:
		return "memcpy";
		break;
	case 14:
		return "memset";
		break;
	case 16:
		return "bcopy";
		break;
	case 17:
		return "bzero";
		break;
	case 19:
		return "sprintf";
		break;
	case 21:
		return "strchr";
		break;
	case 22:
		return "strcmp";
		break;
	case 23:
		return "strcpy";
		break;
	case 24:
		return "strcspn";
		break;
	case 27:
		return "strlen";
		break;
	case 29:
		return "strncmp";
		break;
	case 30:
		return "strncpy";
		break;
	case 32:
		return "strrchr";
		break;
	case 34:
		return "strstr";
		break;
	case 36:
		return "strtol";
		break;
	case 41:
		return "wmemset";
		break;
	default:
		return "unknown";
		break;
	}
}

void CSysclib::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = toupper(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = tolower(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = __look_ctype_table(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 11:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__memcmp(
			reinterpret_cast<void*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
			reinterpret_cast<void*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV0]),
			context.m_State.nGPR[CMIPS::A2].nV0
			));
		break;
	case 12:
		context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
		__memcpy(
			&m_ram[context.m_State.nGPR[CMIPS::A0].nV0],
			&m_ram[context.m_State.nGPR[CMIPS::A1].nV0],
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 13:
		context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
		__memmove(
			&m_ram[context.m_State.nGPR[CMIPS::A0].nV0],
			&m_ram[context.m_State.nGPR[CMIPS::A1].nV0],
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 14:
		context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
		__memset(
			&m_ram[context.m_State.nGPR[CMIPS::A0].nV0],
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 16:
		//bcopy
		memmove(
			&m_ram[context.m_State.nGPR[CMIPS::A1].nV0],
			&m_ram[context.m_State.nGPR[CMIPS::A0].nV0],
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 17:
		//bzero
		__memset(
			&m_ram[context.m_State.nGPR[CMIPS::A0].nV0],
			0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 19:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__sprintf(context));
		break;
	case 21:
		context.m_State.nGPR[CMIPS::V0].nD0 = __strchr(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case 22:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__strcmp(
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV0])
			));
		break;
	case 23:
		context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
		__strcpy(
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV0])
			);
		break;
	case 24:
		context.m_State.nGPR[CMIPS::V0].nD0 = __strcspn(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case 27:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__strlen(
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
			));
		break;
	case 29:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__strncmp(
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV0]),
			context.m_State.nGPR[CMIPS::A2].nV0));
		break;
	case 30:
		context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
		__strncpy(
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV0]),
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 32:
		context.m_State.nGPR[CMIPS::V0].nD0 = __strrchr(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case 34:
		context.m_State.nGPR[CMIPS::V0].nD0 = __strstr(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case 36:
		assert(context.m_State.nGPR[CMIPS::A1].nV0 == 0);
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__strtol(
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
			context.m_State.nGPR[CMIPS::A2].nV0
			));
		break;
	case 41:
		//wmemset
		{
			uint32* dest = reinterpret_cast<uint32*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]);
			uint32 value = context.m_State.nGPR[CMIPS::A1].nV0;
			uint32 numBytes = context.m_State.nGPR[CMIPS::A2].nV0;
			uint32* end = dest + (numBytes / 4);
			while(dest < end)
			{
				*dest++ = value;
			}
			context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nV0;
		}
		break;
	default:
		printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
		assert(0);
		break;
	}
}

uint32 CSysclib::__look_ctype_table(uint32 character)
{
	static const uint8 ctype_table[128] =
	{
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
		0x20, 0x08, 0x08, 0x08, 0x08, 0x08, 0x20, 0x20,
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

		0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
		0x04, 0x04, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

		0x10, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10,

		0x10, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
		0x02, 0x02, 0x02, 0x10, 0x10, 0x10, 0x10, 0x20
	};

	assert(character < 128);

	return ctype_table[character & 0x7F];
}

uint32 CSysclib::__memcmp(const void* dst, const void* src, uint32 length)
{
	return static_cast<uint32>(memcmp(dst, src, length));
}

void CSysclib::__memcpy(void* dest, const void* src, unsigned int length)
{
	memcpy(dest, src, length);
}

void CSysclib::__memmove(void* dest, const void* src, uint32 length)
{
	memmove(dest, src, length);
}

void CSysclib::__memset(void* dest, int character, unsigned int length)
{
	memset(dest, character, length);
}

uint32 CSysclib::__sprintf(CMIPS& context)
{
	CArgumentIterator args(context);
	char* destination = reinterpret_cast<char*>(&m_ram[args.GetNext()]);
	std::string output = m_stdio.PrintFormatted(args);
	strcpy(destination, output.c_str());
	return static_cast<uint32>(output.length());
}

uint32 CSysclib::__strlen(const char* string)
{
	return static_cast<uint32>(strlen(string));
}

uint32 CSysclib::__strcmp(const char* s1, const char* s2)
{
	return static_cast<uint32>(strcmp(s1, s2));
}

void CSysclib::__strcpy(char* dst, const char* src)
{
	strcpy(dst, src);
}

uint32 CSysclib::__strncmp(const char* s1, const char* s2, uint32 length)
{
	return static_cast<uint32>(strncmp(s1, s2, length));
}

void CSysclib::__strncpy(char* dst, const char* src, unsigned int count)
{
	strncpy(dst, src, count);
}

uint32 CSysclib::__strchr(uint32 strPtr, uint32 character)
{
	auto str = reinterpret_cast<const char*>(m_ram + strPtr);
	auto result = strchr(str, static_cast<int>(character));
	if(result == nullptr) return 0;
	size_t ptrDiff = result - str;
	return strPtr + ptrDiff;
}

uint32 CSysclib::__strrchr(uint32 strPtr, uint32 character)
{
	auto str = reinterpret_cast<const char*>(m_ram + strPtr);
	auto result = strrchr(str, static_cast<int>(character));
	if(result == nullptr) return 0;
	size_t ptrDiff = result - str;
	return strPtr + ptrDiff;
}

uint32 CSysclib::__strstr(uint32 str1Ptr, uint32 str2Ptr)
{
	auto str1 = reinterpret_cast<const char*>(m_ram + str1Ptr);
	auto str2 = reinterpret_cast<const char*>(m_ram + str2Ptr);
	auto result = strstr(str1, str2);
	if(result == nullptr) return 0;
	size_t ptrDiff = result - str1;
	return str1Ptr + ptrDiff;
}

uint32 CSysclib::__strcspn(uint32 str1Ptr, uint32 str2Ptr)
{
	auto str1 = reinterpret_cast<const char*>(m_ram + str1Ptr);
	auto str2 = reinterpret_cast<const char*>(m_ram + str2Ptr);
	auto result = strcspn(str1, str2);
	return result;
}

uint32 CSysclib::__strtol(const char* string, unsigned int radix)
{
	return strtol(string, NULL, radix);
}
