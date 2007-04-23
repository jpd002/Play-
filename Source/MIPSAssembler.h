#ifndef _MIPSASSEMBLER_H_
#define _MIPSASSEMBLER_H_

#include "Types.h"

class CMIPSAssembler
{
public:
				        CMIPSAssembler(uint32*);
				        ~CMIPSAssembler();

    unsigned int        GetProgramSize();

	void		        ADDIU(unsigned int, unsigned int, uint16);
	void		        ADDU(unsigned int, unsigned int, unsigned int);
	void		        AND(unsigned int, unsigned int, unsigned int);
	void		        ANDI(unsigned int, unsigned int, uint16);
	void		        BEQ(unsigned int, unsigned int, uint16);
	void		        BGEZ(unsigned int, uint16);
	void		        BGTZ(unsigned int, uint16);
	void		        BNE(unsigned int, unsigned int, uint16);
	void		        BLEZ(unsigned int, uint16);
	void		        BLTZ(unsigned int, uint16);
	void		        DADDU(unsigned int, unsigned int, unsigned int);
	void		        DADDIU(unsigned int, unsigned int, uint16);
	void		        DSLL(unsigned int, unsigned int, unsigned int);
	void		        DSLL32(unsigned int, unsigned int, unsigned int);
	void		        DSLLV(unsigned int, unsigned int, unsigned int);
	void		        DSRA(unsigned int, unsigned int, unsigned int);
	void		        DSRA32(unsigned int, unsigned int, unsigned int);
	void		        DSRAV(unsigned int, unsigned int, unsigned int);
	void		        DSRL(unsigned int, unsigned int, unsigned int);
	void		        DSRL32(unsigned int, unsigned int, unsigned int);
	void		        DSRLV(unsigned int, unsigned int, unsigned int);
	void		        DSUBU(unsigned int, unsigned int, unsigned int);
	void		        ERET();
	void		        JR(unsigned int);
	void		        JALR(unsigned int, unsigned int = 31);
	void		        LD(unsigned int, uint16, unsigned int);
	void		        LDL(unsigned int, uint16, unsigned int);
	void		        LDR(unsigned int, uint16, unsigned int);
    void                LHU(unsigned int, uint16, unsigned int);
    void		        LUI(unsigned int, uint16);
	void		        LW(unsigned int, uint16, unsigned int);
	void		        LWL(unsigned int, uint16, unsigned int);
	void		        LWR(unsigned int, uint16, unsigned int);
	void		        LQ(unsigned int, uint16, unsigned int);
	void		        MFC0(unsigned int, unsigned int);
	void		        MTC0(unsigned int, unsigned int);
    void                MULT(unsigned int, unsigned int, unsigned int);
	void		        MULTU(unsigned int, unsigned int, unsigned int);
	void		        NOP();
    void                OR(unsigned int, unsigned int, unsigned int);
	void		        ORI(unsigned int, unsigned int, uint16);
	void		        SD(unsigned int, uint16, unsigned int);
	void		        SLL(unsigned int, unsigned int, unsigned int);
	void		        SLLV(unsigned int, unsigned int, unsigned int);
    void                SLTIU(unsigned int, unsigned int, uint16);
	void		        SLTU(unsigned int, unsigned int, unsigned int);
	void		        SRA(unsigned int, unsigned int, unsigned int);
	void		        SRAV(unsigned int, unsigned int, unsigned int);
	void		        SRL(unsigned int, unsigned int, unsigned int);
	void		        SRLV(unsigned int, unsigned int, unsigned int);
	void		        SQ(unsigned int, uint16, unsigned int);
	void		        SW(unsigned int, uint16, unsigned int);
	void		        SYSCALL();

    void                AssembleString(const char*);

    static unsigned int GetRegisterIndex(const char*);

private:
	uint32*		        m_pPtr;
    uint32*             m_pStartPtr;
};

#endif
