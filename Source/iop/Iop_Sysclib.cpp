#include "Iop_Sysclib.h"

using namespace Iop;
using namespace std;

CSysclib::CSysclib(uint8* ram, CStdio& stdio) :
m_ram(ram),
m_stdio(stdio)
{

}

CSysclib::~CSysclib()
{

}

string CSysclib::GetId() const
{
    return "sysclib";
}

string CSysclib::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CSysclib::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 12:
        context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
        __memcpy(
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
    case 17:
        __memset(
            &m_ram[context.m_State.nGPR[CMIPS::A0].nV0],
            0,
            context.m_State.nGPR[CMIPS::A1].nV0);
        break;
    case 19:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__sprintf(context));
        break;
    case 23:
        context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
        __strcpy(
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV0])
            );
        break;
    case 27:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__strlen(
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
            ));
        break;
    case 30:
        context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
        __strncpy(
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV0]),
            context.m_State.nGPR[CMIPS::A2].nV0);
        break;
    case 36:
        assert(context.m_State.nGPR[CMIPS::A1].nV0 == 0);
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(__strtol(
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]),
            context.m_State.nGPR[CMIPS::A2].nV0
            ));
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        assert(0);
        break;
    }
}

void CSysclib::__memcpy(void* dest, const void* src, unsigned int length)
{
    memcpy(dest, src, length);
}

void CSysclib::__memset(void* dest, int character, unsigned int length)
{
    memset(dest, character, length);
}

uint32 CSysclib::__sprintf(CMIPS& context)
{
    CArgumentIterator args(context);
    char* destination = reinterpret_cast<char*>(&m_ram[args.GetNext()]);
    string output = m_stdio.PrintFormatted(args);
    strcpy(destination, output.c_str());
    return static_cast<uint32>(output.length());
}

uint32 CSysclib::__strlen(const char* string)
{
    return static_cast<uint32>(strlen(string));
}

void CSysclib::__strcpy(char* dst, const char* src)
{
    strcpy(dst, src);
}

void CSysclib::__strncpy(char* dst, const char* src, unsigned int count)
{
    strncpy(dst, src, count);
}

uint32 CSysclib::__strtol(const char* string, unsigned int radix)
{
    return strtol(string, NULL, radix);
}
