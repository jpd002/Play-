#include "MIPSAssembler.h"
#include "MipsAssemblerDefinitions.h"
#include "MIPS.h"
#include <boost/tokenizer.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <limits.h>
#include "lexical_cast_ex.h"

using namespace boost;
using namespace std;

CMIPSAssembler::CMIPSAssembler(uint32* pPtr) :
m_nextLabelId(1),
m_pPtr(pPtr),
m_pStartPtr(pPtr)
{

}

CMIPSAssembler::~CMIPSAssembler()
{
	ResolveLabelReferences();
}

unsigned int CMIPSAssembler::GetProgramSize()
{
    return static_cast<unsigned int>(m_pPtr - m_pStartPtr);
}

CMIPSAssembler::LABEL CMIPSAssembler::CreateLabel()
{
	LABEL newLabel;
	newLabel.id = m_nextLabelId++;
	return newLabel;
}

void CMIPSAssembler::MarkLabel(LABEL label)
{
    m_labels[label] = GetProgramSize();
}

void CMIPSAssembler::CreateLabelReference(LABEL label)
{
    LABELREF reference;
    reference.address = GetProgramSize();
    m_labelReferences.insert(LabelReferenceMapType::value_type(label, reference));
}

void CMIPSAssembler::ResolveLabelReferences()
{
    for(LabelReferenceMapType::iterator labelRef(m_labelReferences.begin());
        m_labelReferences.end() != labelRef; labelRef++)
    {
        LabelMapType::iterator label(m_labels.find(labelRef->first));
        if(label == m_labels.end())
        {
            throw runtime_error("Invalid label.");
        }
        size_t referencePos = labelRef->second.address;
        size_t labelPos = label->second;
        int offset = static_cast<int>(labelPos - referencePos - 1);
		if(offset > SHRT_MAX || offset < SHRT_MIN)
		{
			throw runtime_error("Jump length too long.");
		}
		uint32& instruction = m_pStartPtr[referencePos];
		instruction &= 0xFFFF0000;
		instruction |= static_cast<uint16>(offset);
    }
    m_labelReferences.clear();
}

void CMIPSAssembler::ADDIU(unsigned int nRT, unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x09) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::ADDU(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21)  | (nRT << 16) | (nRD << 11) | (0x21);
	m_pPtr++;
}

void CMIPSAssembler::AND(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21)  | (nRT << 16) | (nRD << 11) | (0x24);
	m_pPtr++;
}

void CMIPSAssembler::ANDI(unsigned int nRT, unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x0C) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::BEQ(unsigned int nRS, unsigned int nRT, uint16 nImmediate)
{
	(*m_pPtr) = ((0x04) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::BEQ(unsigned int nRS, unsigned int nRT, LABEL label)
{
	CreateLabelReference(label);
	BEQ(nRS, nRT, 0);
}

void CMIPSAssembler::BGEZ(unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x01) << 26) | (nRS << 21) | ((0x01) << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::BGTZ(unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x07) << 26) | (nRS << 21) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::BNE(unsigned int nRS, unsigned int nRT, uint16 nImmediate)
{
	(*m_pPtr) = ((0x05) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::BNE(unsigned int nRS, unsigned int nRT, LABEL label)
{
	CreateLabelReference(label);
	BNE(nRS, nRT, 0);
}

void CMIPSAssembler::BLEZ(unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x06) << 26) | (nRS << 21) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::BLTZ(unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x01) << 26) | (nRS << 21) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::DADDU(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | (0x2D);
	m_pPtr++;
}

void CMIPSAssembler::DADDIU(unsigned int nRT, unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x19) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::DSLL(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x38;
	m_pPtr++;
}

void CMIPSAssembler::DSLL32(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x3C;
	m_pPtr++;
}

void CMIPSAssembler::DSLLV(unsigned int nRD, unsigned int nRT, unsigned int nRS)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x14;
	m_pPtr++;
}

void CMIPSAssembler::DSRA(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x3B;
	m_pPtr++;
}

void CMIPSAssembler::DSRA32(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x3F;
	m_pPtr++;
}

void CMIPSAssembler::DSRAV(unsigned int nRD, unsigned int nRT, unsigned int nRS)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x17;
	m_pPtr++;
}

void CMIPSAssembler::DSRL(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x3A;
	m_pPtr++;
}

void CMIPSAssembler::DSRL32(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x3E;
	m_pPtr++;
}

void CMIPSAssembler::DSRLV(unsigned int nRD, unsigned int nRT, unsigned int nRS)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x16;
	m_pPtr++;
}

void CMIPSAssembler::DSUBU(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | (0x2F);
	m_pPtr++;
}

void CMIPSAssembler::ERET()
{
	(*m_pPtr) = 0x42000018;
	m_pPtr++;
}

void CMIPSAssembler::JAL(uint32 address)
{
	(*m_pPtr) = ((0x03) << 26) | ((address >> 2) & 0x03FFFFFF);
	m_pPtr++;
}

void CMIPSAssembler::JALR(unsigned int nRS, unsigned int nRD)
{
	(*m_pPtr) = (nRS << 21) | (nRD << 11) | (0x09);
	m_pPtr++;
}

void CMIPSAssembler::JR(unsigned int nRS)
{
	(*m_pPtr) = (nRS << 21) | (0x08);
	m_pPtr++;
}

void CMIPSAssembler::LD(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x37) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::LDL(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x1A) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::LDR(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x1B) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::LHU(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x25) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::LI(unsigned int rt, uint32 immediate)
{
	uint16 low = static_cast<int16>(immediate);
	uint16 high = static_cast<int16>(immediate >> 16);
	LUI(rt, high);
	if(low != 0)
	{
		ORI(rt, rt, low);
	}
}

void CMIPSAssembler::LUI(unsigned int nRT, uint16 nImmediate)
{
	(*m_pPtr) = ((0x0F) << 26) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::LQ(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x1E) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::LW(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x23) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::LWL(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x22) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::LWR(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x26) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::MFC0(unsigned int nRT, unsigned int nRD)
{
	(*m_pPtr) = ((0x10) << 26) | (nRT << 16) | (nRD << 11);
	m_pPtr++;
}

void CMIPSAssembler::MTC0(unsigned int nRT, unsigned int nRD)
{
	(*m_pPtr) = ((0x10) << 26) | ((0x04) << 21) | (nRT << 16) | (nRD << 11);
	m_pPtr++;
}

void CMIPSAssembler::MOV(unsigned int rd, unsigned int rs)
{
	ADDU(rd, rs, 0);
}

void CMIPSAssembler::MULT(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x18;
	m_pPtr++;
}

void CMIPSAssembler::MULTU(unsigned int nRS, unsigned int nRT, unsigned int nRD)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x19;
	m_pPtr++;
}

void CMIPSAssembler::NOP()
{
	(*m_pPtr) = 0;
	m_pPtr++;
}

void CMIPSAssembler::NOR(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x27;
	m_pPtr++;
}

void CMIPSAssembler::OR(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x25;
	m_pPtr++;
}

void CMIPSAssembler::ORI(unsigned int nRT, unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x0D) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::SD(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x3F) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::SLL(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6);
	m_pPtr++;
}

void CMIPSAssembler::SLLV(unsigned int nRD, unsigned int nRT, unsigned int nRS)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x04;
	m_pPtr++;
}

void CMIPSAssembler::SLTI(unsigned int nRT, unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x0A) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::SLTIU(unsigned int nRT, unsigned int nRS, uint16 nImmediate)
{
	(*m_pPtr) = ((0x0B) << 26) | (nRS << 21) | (nRT << 16) | nImmediate;
	m_pPtr++;
}

void CMIPSAssembler::SLTU(unsigned int nRD, unsigned int nRS, unsigned int nRT)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x2B;
	m_pPtr++;
}

void CMIPSAssembler::SRA(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x03;
	m_pPtr++;
}

void CMIPSAssembler::SRAV(unsigned int nRD, unsigned int nRT, unsigned int nRS)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x07;
	m_pPtr++;
}

void CMIPSAssembler::SRL(unsigned int nRD, unsigned int nRT, unsigned int nSA)
{
	nSA &= 0x1F;
	(*m_pPtr) = (nRT << 16) | (nRD << 11) | (nSA << 6) | 0x02;
	m_pPtr++;
}

void CMIPSAssembler::SRLV(unsigned int nRD, unsigned int nRT, unsigned int nRS)
{
	(*m_pPtr) = (nRS << 21) | (nRT << 16) | (nRD << 11) | 0x06;
	m_pPtr++;
}

void CMIPSAssembler::SQ(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x1F) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::SW(unsigned int nRT, uint16 nOffset, unsigned int nBase)
{
	(*m_pPtr) = ((0x2B) << 26) | (nBase << 21) | (nRT << 16) | nOffset;
	m_pPtr++;
}

void CMIPSAssembler::SYSCALL()
{
	(*m_pPtr) = 0x0000000C;
	m_pPtr++;
}

void CMIPSAssembler::AssembleString(const char* sCode)
{
    string sString(sCode);
    tokenizer<> Tokens(sString);

    tokenizer<>::iterator itToken(Tokens.begin());
    const char* sMnemonic;

    //First token must be the instruction mnemonic
    sMnemonic = (*itToken).c_str();

    bool nFound(false);

    for(unsigned int i = 0; ; i++)
    {
        MipsAssemblerDefinitions::Instruction* pInstruction(MipsAssemblerDefinitions::g_Instructions[i]);
        if(pInstruction == NULL) break;

        if(!strcmp(pInstruction->m_sMnemonic, sMnemonic))
        {
            pInstruction->Invoke(Tokens, itToken, this);
            nFound = true;
            break;
        }
    }

    if(nFound == false)
    {
        throw runtime_error("Invalid mnemonic specified.");
    }
}

unsigned int CMIPSAssembler::GetRegisterIndex(const char* sRegisterName)
{
    unsigned int nRegister;

    nRegister = -1;
    for(unsigned int i = 0; i < 32; i++)
    {
        if(!strcmp(CMIPS::m_sGPRName[i], sRegisterName))
        {
            nRegister = i;
            break;
        }
    }

    return nRegister;
}
