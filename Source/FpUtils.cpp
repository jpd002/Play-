#include "FpUtils.h"
#include "MipsJitter.h"

#ifdef _WIN32
#define DENORM_X86
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#define DENORM_X86
#elif TARGET_CPU_ARM64
#define DENORM_AARCH64
#endif
#elif defined(__ANDROID__) || defined(__linux__) || defined(__FreeBSD__)
#if defined(__i386__) || defined(__x86_64__)
#define DENORM_X86
#elif defined(__aarch64__)
#define DENORM_AARCH64
#endif
#endif

#ifdef DENORM_X86
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

#define EXC_FP_MAX 0x7F7FFFFF

void FpUtils::SetDenormalHandlingMode()
{
#ifdef DENORM_X86
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif
#ifdef DENORM_AARCH64
	static const uint64 fpcrFZ = (1 << 24);
	__asm__ __volatile__(
	    "mrs x0, fpcr\n"
	    "orr x0, x0, %0\n"
	    "msr fpcr, x0\n"
	    :
	    : "ri"(fpcrFZ)
	    : "x0");
#endif
}

void FpUtils::EnableFpExceptions()
{
#ifdef _WIN32
	unsigned int currentState = 0;
	_controlfp_s(&currentState, _MCW_EM & ~(_EM_ZERODIVIDE | _EM_INVALID), _MCW_EM);
#endif
}

void FpUtils::IsZero(CMipsJitter* codeGen, size_t offset)
{
	//Check wether an FP number is +/-0
	//If the exponent is 0, then we have 0 (denormals count as 0)
	//BeginIf(CONDITION_EQ) to verify result
	codeGen->PushRel(offset);
	codeGen->PushCst(0x7F800000);
	codeGen->And();
	codeGen->PushCst(0);
}

void FpUtils::IsNaN(CMipsJitter* codeGen, size_t offset)
{
	//Check wether an FP number is a NaN (exponent is 0xFF)
	//BeginIf(CONDITION_EQ) to verify result
	codeGen->PushRel(offset);
	codeGen->PushCst(0x7F800000);
	codeGen->And();
	codeGen->PushCst(0x7F800000);
}

void FpUtils::AssertIsNotNaN(CMipsJitter* codeGen, size_t offset)
{
	FpUtils::IsNaN(codeGen, offset);
	codeGen->BeginIf(Jitter::CONDITION_EQ);
	{
		codeGen->Break();
	}
	codeGen->EndIf();
}

void FpUtils::ComputeDivisionByZero(CMipsJitter* codeGen, size_t dividend, size_t divisor)
{
	//Return either +/-FP_MAX
	codeGen->PushCst(EXC_FP_MAX);
	codeGen->PushRel(dividend);
	codeGen->PushRel(divisor);
	codeGen->Xor();
	codeGen->PushCst(0x80000000);
	codeGen->And();
	codeGen->Or();
}
