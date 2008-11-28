#include "Iop_Stdio.h"
#include <boost/lexical_cast.hpp>
#include "lexical_cast_ex.h"

using namespace Iop;
using namespace std;
using namespace boost;

CStdio::CStdio(uint8* ram) :
m_ram(ram)
{

}

CStdio::~CStdio()
{

}

string CStdio::GetId() const
{
    return "stdio";
}

string CStdio::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CStdio::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        __printf(context);
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

string CStdio::PrintFormatted(CArgumentIterator& args)
{
    string output;
    const char* format = reinterpret_cast<const char*>(&m_ram[args.GetNext()]);
    while(*format != 0)
    {
        char character = *(format++);
        if(character == '%')
        {
            bool paramDone = false;
            bool inPrecision = false;
            string precision;
            while(!paramDone && *format != 0) 
            {
                char type = *(format++);
                if(type == 's')
                {
                    const char* text = reinterpret_cast<const char*>(&m_ram[args.GetNext()]);
                    output += text;
                    paramDone = true;
                }
                else if(type == 'd')
                {
                    int number = args.GetNext();
                    unsigned int precisionValue = precision.length() ? lexical_cast<unsigned int>(precision) : 0;
                    output += lexical_cast<string>(number);
                    paramDone = true;
                }
                else if(type == 'u')
                {
                    unsigned int number = args.GetNext();
                    unsigned int precisionValue = precision.length() ? lexical_cast<unsigned int>(precision) : 0;
                    output += lexical_cast_uint<string>(number, precisionValue);
                    paramDone = true;
                }
                else if(type == '.')
                {
                    inPrecision = true;
                }
                else
                {
                    if(inPrecision)
                    {
                        precision += type;
                    }
                }
            }
        }
        else
        {
            output += character;
        }
    }
    return output;
}

void CStdio::__printf(CMIPS& context)
{
    CArgumentIterator args(context);
    string output = PrintFormatted(args);
    printf("%s", output.c_str());
}
