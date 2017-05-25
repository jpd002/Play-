#include "Iop_Sysclib.h"
#include "../Ps2Const.h"

using namespace Iop;

CSysclib::CSysclib(uint8* ram, uint8* spr, CStdio& stdio)
: m_ram(ram)
, m_spr(spr)
, m_stdio(stdio)
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
	case 4:
		return "setjmp";
		break;
	case 5:
		return "longjmp";
		break;
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
	case 20:
		return "strcat";
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
	case 35:
		return "strtok";
		break;
	case 36:
		return "strtol";
		break;
	case 40:
		return "wmemcopy";
		break;
	case 41:
		return "wmemset";
		break;
	case 42:
		return "vsprintf";
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
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = __setjmp(context);
		break;
	case 5:
		__longjmp(context);
		break;
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
		context.m_State.nGPR[CMIPS::V0].nD0 = __memset(
			context.m_State.nGPR[CMIPS::A0].nV0,
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
			context.m_State.nGPR[CMIPS::A0].nV0,
			0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 19:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__sprintf(context));
		break;
	case 20:
		context.m_State.nGPR[CMIPS::V0].nD0 = __strcat(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			);
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
	case 35:
		context.m_State.nGPR[CMIPS::V0].nD0 = __strtok(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case 36:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__strtol(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0
			));
		break;
	case 40:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__wmemcopy(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
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
	case 42:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__vsprintf(
			context,
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0
			));
		break;
	default:
		printf("%s(%08X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
		assert(0);
		break;
	}
}

uint8* CSysclib::GetPtr(uint32 ptr, uint32 size) const
{
	assert(ptr != 0);
	if(ptr >= PS2::IOP_SCRATCH_ADDR)
	{
		//Some games (Phantasy Star Collection) seem to address areas beyond the SPR's limits
		ptr &= (PS2::IOP_SCRATCH_SIZE - 1);
		assert((ptr + size) <= PS2::IOP_SCRATCH_SIZE);
		return reinterpret_cast<uint8*>(m_spr + ptr);
	}
	else
	{
		//We should rarely get addresses that point to other areas
		//than RAM, so we assert just to give a warning because it might
		//mean there's an error somewhere else
		assert(ptr < PS2::IOP_RAM_SIZE);
		ptr &= (PS2::IOP_RAM_SIZE - 1);
		return reinterpret_cast<uint8*>(m_ram + ptr);
	}
}

int32 CSysclib::__setjmp(CMIPS& context)
{
	uint32 envPtr = context.m_State.nGPR[CMIPS::A0].nV0;
	auto env = reinterpret_cast<JMP_BUF*>(GetPtr(envPtr, sizeof(JMP_BUF)));
	env->ra = context.m_State.nGPR[CMIPS::RA].nV0;
	env->sp = context.m_State.nGPR[CMIPS::SP].nV0;
	env->fp = context.m_State.nGPR[CMIPS::FP].nV0;
	env->s0 = context.m_State.nGPR[CMIPS::S0].nV0;
	env->s1 = context.m_State.nGPR[CMIPS::S1].nV0;
	env->s2 = context.m_State.nGPR[CMIPS::S2].nV0;
	env->s3 = context.m_State.nGPR[CMIPS::S3].nV0;
	env->s4 = context.m_State.nGPR[CMIPS::S4].nV0;
	env->s5 = context.m_State.nGPR[CMIPS::S5].nV0;
	env->s6 = context.m_State.nGPR[CMIPS::S6].nV0;
	env->s7 = context.m_State.nGPR[CMIPS::S7].nV0;
	env->gp = context.m_State.nGPR[CMIPS::GP].nV0;
	return 0;
}

void CSysclib::__longjmp(CMIPS& context)
{
	uint32 envPtr = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 returnValue = context.m_State.nGPR[CMIPS::A1].nV0;
	auto env = reinterpret_cast<const JMP_BUF*>(GetPtr(envPtr, sizeof(JMP_BUF)));
	context.m_State.nPC = env->ra;
	context.m_State.nGPR[CMIPS::SP].nV0 = env->sp;
	context.m_State.nGPR[CMIPS::FP].nV0 = env->fp;
	context.m_State.nGPR[CMIPS::S0].nV0 = env->s0;
	context.m_State.nGPR[CMIPS::S1].nV0 = env->s1;
	context.m_State.nGPR[CMIPS::S2].nV0 = env->s2;
	context.m_State.nGPR[CMIPS::S3].nV0 = env->s3;
	context.m_State.nGPR[CMIPS::S4].nV0 = env->s4;
	context.m_State.nGPR[CMIPS::S5].nV0 = env->s5;
	context.m_State.nGPR[CMIPS::S6].nV0 = env->s6;
	context.m_State.nGPR[CMIPS::S7].nV0 = env->s7;
	context.m_State.nGPR[CMIPS::GP].nV0 = env->gp;
	context.m_State.nGPR[CMIPS::V0].nV0 = static_cast<int32>(returnValue);
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

uint32 CSysclib::__memset(uint32 destPtr, uint32 character, uint32 length)
{
	auto dest = reinterpret_cast<uint8*>(GetPtr(destPtr, length));
	memset(dest, character, length);
	return destPtr;
}

uint32 CSysclib::__sprintf(CMIPS& context)
{
	CCallArgumentIterator args(context);
	auto destination = reinterpret_cast<char*>(m_ram + args.GetNext());
	auto format = reinterpret_cast<const char*>(m_ram + args.GetNext());
	auto output = m_stdio.PrintFormatted(format, args);
	strcpy(destination, output.c_str());
	return static_cast<uint32>(output.length());
}

uint32 CSysclib::__strcat(uint32 dstPtr, uint32 srcPtr)
{
	assert(dstPtr != 0);
	assert(srcPtr != 0);
	auto dst = reinterpret_cast<char*>(m_ram + dstPtr);
	auto src = reinterpret_cast<const char*>(m_ram + srcPtr);
	strcat(dst, src);
	return dstPtr;
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

uint32 CSysclib::__strtok(uint32 sPtr, uint32 delimPtr)
{
	auto delim = reinterpret_cast<const char*>(m_ram + delimPtr);

	if(sPtr != 0)
	{
		m_strtok_prevSPtr = sPtr;
	}
	else if(m_strtok_prevSPtr == 0)
	{
		//If sPtr == 0 && prevSPtr == 0
		return 0;
	}
	auto s = reinterpret_cast<char*>(m_ram + m_strtok_prevSPtr);

	auto str = s + strspn(s, delim);
	s = str + strcspn(str, delim);
	if(s == str)
	{
		m_strtok_prevSPtr = 0;
		return 0;
	}
	if(*s)
	{
		(*s) = 0;
		m_strtok_prevSPtr = (reinterpret_cast<uint8*>(s) - m_ram) + 1;
	}
	else
	{
		m_strtok_prevSPtr = 0;
	}
	return reinterpret_cast<uint8*>(str) - m_ram;
}

uint32 CSysclib::__strcspn(uint32 str1Ptr, uint32 str2Ptr)
{
	auto str1 = reinterpret_cast<const char*>(m_ram + str1Ptr);
	auto str2 = reinterpret_cast<const char*>(m_ram + str2Ptr);
	auto result = strcspn(str1, str2);
	return result;
}

uint32 CSysclib::__strtol(uint32 stringPtr, uint32 endPtrPtr, uint32 radix)
{
	auto string = reinterpret_cast<const char*>(GetPtr(stringPtr, 0));
	char* end = nullptr;
	uint32 result = strtol(string, &end, radix);
	if(endPtrPtr != 0)
	{
		auto endPtr = reinterpret_cast<uint32*>(GetPtr(endPtrPtr, 4));
		(*endPtr) = static_cast<uint32>(end - string);
	}
	return result;
}

uint32 CSysclib::__wmemcopy(uint32 dstPtr, uint32 srcPtr, uint32 size)
{
	assert((size & 0x3) == 0);
	auto dst = reinterpret_cast<uint8*>(m_ram + dstPtr);
	auto src = reinterpret_cast<uint8*>(m_ram + srcPtr);
	memmove(dst, src, size);
	return dstPtr;
}

uint32 CSysclib::__vsprintf(CMIPS& context, uint32 destinationPtr, uint32 formatPtr, uint32 argsPtr)
{
	CValistArgumentIterator args(context, argsPtr);
	auto destination = reinterpret_cast<char*>(m_ram + destinationPtr);
	auto format = reinterpret_cast<const char*>(m_ram + formatPtr);
	auto output = m_stdio.PrintFormatted(format, args);
	strcpy(destination, output.c_str());
	return static_cast<uint32>(output.length());
}
