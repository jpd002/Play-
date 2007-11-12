#include "Iop_Sysclib.h"

using namespace Iop;
using namespace std;

CSysclib::CSysclib(uint8* ram) :
m_ram(ram)
{

}

CSysclib::~CSysclib()
{

}

string CSysclib::GetId() const
{
    return "sysclib";
}

void CSysclib::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 14:
        context.m_State.nGPR[CMIPS::V0].nD0 = context.m_State.nGPR[CMIPS::A0].nD0;
        __memset(
            &m_ram[context.m_State.nGPR[CMIPS::A0].nV0],
            context.m_State.nGPR[CMIPS::A1].nV0,
            context.m_State.nGPR[CMIPS::A2].nV0);
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
        break;
    }
}

void CSysclib::__memset(void* dest, int character, unsigned int length)
{
    memset(dest, character, length);
}

uint32 CSysclib::__strlen(const char* string)
{
    return static_cast<uint32>(strlen(string));
}

void CSysclib::__strncpy(char* dst, const char* src, unsigned int count)
{
    strncpy(dst, src, count);
}

uint32 CSysclib::__strtol(const char* string, unsigned int radix)
{
    return strtol(string, NULL, radix);
}
