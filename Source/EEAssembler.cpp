#include "EEAssembler.h"

CEEAssembler::CEEAssembler(uint32* ptr)
: CMIPSAssembler(ptr)
{

}

void CEEAssembler::LQ(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x1E) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}

void CEEAssembler::MFHI1(unsigned int rd)
{
	(*m_ptr) = ((0x1C) << 26) | (rd << 11) | (0x10);
	m_ptr++;
}

void CEEAssembler::MFLO1(unsigned int rd)
{
	(*m_ptr) = ((0x1C) << 26) | (rd << 11) | (0x12);
	m_ptr++;
}

void CEEAssembler::MTHI1(unsigned int rs)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (0x11);
	m_ptr++;
}

void CEEAssembler::MTLO1(unsigned int rs)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (0x13);
	m_ptr++;
}

void CEEAssembler::PADDW(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x00) << 6) | (0x08);
	m_ptr++;
}

void CEEAssembler::PEXCH(unsigned int rd, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rt << 16) | (rd << 11) | ((0x1A) << 6) | (0x29);
	m_ptr++;
}

void CEEAssembler::PEXTLB(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x1A) << 6) | (0x08);
	m_ptr++;
}

void CEEAssembler::PEXTUB(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x1A) << 6) | (0x28);
	m_ptr++;
}

void CEEAssembler::PEXTLH(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x16) << 6) | (0x08);
	m_ptr++;
}

void CEEAssembler::PEXTUH(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x16) << 6) | (0x28);
	m_ptr++;
}

void CEEAssembler::PEXCW(unsigned int rd, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rt << 16) | (rd << 11) | ((0x1E) << 6) | (0x29);
	m_ptr++;
}

void CEEAssembler::PMFLO(unsigned int rd)
{
	(*m_ptr) = ((0x1C) << 26) | (rd << 11) | ((0x09) << 6) | (0x09);
	m_ptr++;
}

void CEEAssembler::PMFHI(unsigned int rd)
{
	(*m_ptr) = ((0x1C) << 26) | (rd << 11) | ((0x08) << 6) | (0x09);
	m_ptr++;
}

void CEEAssembler::PMFHL_UW(unsigned int rd)
{
	(*m_ptr) = ((0x1C) << 26) | (rd << 11) | ((0x01) << 6) | (0x30);
	m_ptr++;
}

void CEEAssembler::PMFHL_LH(unsigned int rd)
{
	(*m_ptr) = ((0x1C) << 26) | (rd << 11) | ((0x03) << 6) | (0x30);
	m_ptr++;
}

void CEEAssembler::PMULTH(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x1C) << 6) | (0x09);
	m_ptr++;
}

void CEEAssembler::PPACH(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x17) << 6) | (0x08);
	m_ptr++;
}

void CEEAssembler::PPACW(unsigned int rd, unsigned int rs, unsigned int rt)
{
	(*m_ptr) = ((0x1C) << 26) | (rs << 21) | (rt << 16) | (rd << 11) | ((0x13) << 6) | (0x08);
	m_ptr++;
}

void CEEAssembler::SQ(unsigned int rt, uint16 offset, unsigned int base)
{
	(*m_ptr) = ((0x1F) << 26) | (base << 21) | (rt << 16) | offset;
	m_ptr++;
}
