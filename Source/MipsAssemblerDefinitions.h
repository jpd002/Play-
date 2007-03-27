#ifndef _MIPSASSEMBLERDEFINITIONS_H_
#define _MIPSASSEMBLERDEFINITIONS_H_

#include <boost/tokenizer.hpp>

class CMIPSAssembler;

namespace MipsAssemblerDefinitions
{
    struct Instruction
    {
        Instruction(const char* sMnemonic) :
        m_sMnemonic(sMnemonic)
        {

        }

        const char* m_sMnemonic;
        virtual void Invoke(boost::tokenizer<>&, boost::tokenizer<>::iterator&, CMIPSAssembler*) = 0;
    };

    extern Instruction* g_Instructions[];
};

#endif
