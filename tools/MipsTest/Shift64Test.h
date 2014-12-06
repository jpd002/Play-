#ifndef _SHIFT64TEST_H_
#define _SHIFT64TEST_H_

#include "Test.h"
#include "MIPSAssembler.h"

class CShift64Test : public CTest
{
public:
	void		Execute(CTestVm&);

private:
	typedef void (CMIPSAssembler::*ShiftAssembleFunction)(unsigned int, unsigned int, unsigned int);
	typedef void (CMIPSAssembler::*VariableShiftAssembleFunction)(unsigned int, unsigned int, unsigned int);
	typedef std::function<uint64 (uint64, unsigned int)> ShiftFunction;

	void		TestShift(CTestVm&, const ShiftAssembleFunction&, const ShiftFunction&);
	void		TestVariableShift(CTestVm&, const VariableShiftAssembleFunction&, const ShiftFunction&);
};

#endif
