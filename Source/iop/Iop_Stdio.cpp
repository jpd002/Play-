#include "Iop_Stdio.h"
#include <boost/lexical_cast.hpp>
#include "lexical_cast_ex.h"
#include "../Log.h"

#define LOG_NAME            "iop_stdio"

#define FUNCTION_PRINTF     "printf"

using namespace Iop;
using namespace std;

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
    switch(functionId)
    {
    case 4:
        return FUNCTION_PRINTF;
        break;
    default:
    	return "unknown";
        break;
    }
}

void CStdio::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        __printf(context);
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called. PC = (%0.8X).", 
            functionId, context.m_State.nPC);
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
                    unsigned int precisionValue = precision.length() ? boost::lexical_cast<unsigned int>(precision) : 0;
                    output += boost::lexical_cast<string>(number);
                    paramDone = true;
                }
                else if(type == 'u')
                {
                    unsigned int number = args.GetNext();
                    unsigned int precisionValue = precision.length() ? boost::lexical_cast<unsigned int>(precision) : 0;
                    output += lexical_cast_uint<string>(number, precisionValue);
                    paramDone = true;
                }
                else if(type == 'x' || type == 'X')
                {
                    uint32 number = args.GetNext();
                    unsigned int precisionValue = precision.length() ? boost::lexical_cast<unsigned int>(precision) : 0;
                    output += lexical_cast_hex<string>(number, precisionValue);
                    paramDone = true;
                }
                else if(type == 'l')
                {
                    //Length specifier, don't bother about it.
                }
                else if(type == '.')
                {
                    inPrecision = true;
                }
                else
                {
                    assert(isdigit(type));
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
