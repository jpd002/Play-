#include "Iop_Stdio.h"

using namespace Iop;
using namespace std;

CStdio::CStdio()
{

}

CStdio::~CStdio()
{

}

string CStdio::GetId() const
{
    return "stdio";
}

void CStdio::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        Printf(context);
        break;
    default:
        printf("%s: Unknown function (%d) called.", __FUNCTION__, functionId);
        break;
    }
}

void CStdio::Printf(CMIPS& context)
{
    const char* format = reinterpret_cast<const char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV[0]]);
    unsigned int param = CMIPS::A1;
    while(*format != 0)
    {
        char character = *(format++);
        if(character == '%')
        {
            char type = *(format++);
            if(type == 's')
            {
                const char* text = reinterpret_cast<const char*>(&m_ram[context.m_State.nGPR[param++].nV[0]]);
                printf("%s", text);
            }
            else if(type == 'd')
            {
                int number = context.m_State.nGPR[param++].nV[0];
                printf("%d", number);
            }
        }
        else
        {
            putc(character, stdout);
        }
    }
}
