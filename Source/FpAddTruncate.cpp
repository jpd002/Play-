//Source: The LLVM Compiler Infrastructure - lib/addsf3.c
//Modified to truncate result of addition

#include <limits.h>
#include <boost/cstdint.hpp>
#include "FpAddTruncate.h"
#include "BitManip.h"

typedef uint32 rep_t;
typedef int32 srep_t;
typedef float fp_t;
#define REP_C UINT32_C
#define significandBits 23

#define typeWidth       (sizeof(rep_t)*CHAR_BIT)
#define exponentBits    (typeWidth - significandBits - 1)
#define maxExponent     ((1 << exponentBits) - 1)
#define exponentBias    (maxExponent >> 1)

#define implicitBit     (REP_C(1) << significandBits)
#define significandMask (implicitBit - 1U)
#define signBit         (REP_C(1) << (significandBits + exponentBits))
#define absMask         (signBit - 1U)
#define exponentMask    (absMask ^ significandMask)
#define oneRep          ((rep_t)exponentBias << significandBits)
#define infRep          exponentMask
#define quietBit        (implicitBit >> 1)
#define qnanRep         (exponentMask | quietBit)

static inline int rep_clz(rep_t a) {
	return __builtin_clz(a);
}

uint32 FpAddTruncate(uint32 a, uint32 b) 
{
    const rep_t aAbs = a & absMask;
    const rep_t bAbs = b & absMask;
    
    // Detect if a or b is zero, infinity, or NaN.
    if (aAbs - 1U >= infRep - 1U || bAbs - 1U >= infRep - 1U) {
        
        // NaN + anything = qNaN
        if (aAbs > infRep) return (a | quietBit);
        // anything + NaN = qNaN
        if (bAbs > infRep) return (b | quietBit);
        
        if (aAbs == infRep) {
            // +/-infinity + -/+infinity = qNaN
            if ((a ^ b) == signBit) return qnanRep;
            // +/-infinity + anything remaining = +/- infinity
            else return a;
        }
        
        // anything remaining + +/-infinity = +/-infinity
        if (bAbs == infRep) return b;
        
        // zero + anything = anything
        if (!aAbs) {
            // but we need to get the sign right for zero + zero
            if (!bAbs) return (a & b);
            else return b;
        }
        
        // anything + zero = anything
        if (!bAbs) return a;
    }
    
    // Swap a and b if necessary so that a has the larger absolute value.
    if (bAbs > aAbs) {
        const uint32 temp = a;
        a = b;
        b = temp;
    }
    
    // Extract the exponent and significand from the (possibly swapped) a and b.
    int aExponent = a >> significandBits & maxExponent;
    int bExponent = b >> significandBits & maxExponent;
    rep_t aSignificand = a & significandMask;
    rep_t bSignificand = b & significandMask;
    
    // Normalize any denormals, and adjust the exponent accordingly.
    //if (aExponent == 0) aExponent = normalize(&aSignificand);
    //if (bExponent == 0) bExponent = normalize(&bSignificand);
    
    // The sign of the result is the sign of the larger operand, a.  If they
    // have opposite signs, we are performing a subtraction; otherwise addition.
    const rep_t resultSign = a & signBit;
    const bool subtraction = (a ^ b) & signBit;
    
    // Shift the significands to give us round, guard and sticky, and or in the
    // implicit significand bit.  (If we fell through from the denormal path it
    // was already set by normalize( ), but setting it twice won't hurt
    // anything.)
    aSignificand = (aSignificand | implicitBit) << 3;
    bSignificand = (bSignificand | implicitBit) << 3;
    
    // Shift the significand of b by the difference in exponents, with a sticky
    // bottom bit to get rounding correct.
    const unsigned int align = aExponent - bExponent;
    if (align) {
        if (align < typeWidth) {
            //const bool sticky = bSignificand << (typeWidth - align);
            bSignificand = bSignificand >> align;
        } else {
            bSignificand = 0; // sticky; b is known to be non-zero.
        }
    }
    
    if (subtraction) {
        aSignificand -= bSignificand;
        
        // If a == -b, return +zero.
        if (aSignificand == 0) return 0;
        
        // If partial cancellation occured, we need to left-shift the result
        // and adjust the exponent:
        if (aSignificand < implicitBit << 3) {
            const int shift = rep_clz(aSignificand) - rep_clz(implicitBit << 3);
            aSignificand <<= shift;
            aExponent -= shift;
        }
    }
    
    else /* addition */ {
        aSignificand += bSignificand;
        
        // If the addition carried up, we need to right-shift the result and
        // adjust the exponent:
        if (aSignificand & implicitBit << 4) {
            const bool sticky = aSignificand & 1;
            aSignificand = aSignificand >> 1 | sticky;
            aExponent += 1;
        }
    }
    
    // If we have overflowed the type, return +/- infinity:
    if (aExponent >= maxExponent) return infRep | resultSign;
    
    if (aExponent <= 0) {
        // Result is denormal before rounding; the exponent is zero and we
        // need to shift the significand.
        const int shift = 1 - aExponent;
        const bool sticky = aSignificand << (typeWidth - shift);
        aSignificand = aSignificand >> shift | sticky;
        aExponent = 0;
    }
    
    // Low three bits are round, guard, and sticky.
    const int roundGuardSticky = aSignificand & 0x7;
    
    // Shift the significand into place, and mask off the implicit bit.
    rep_t result = aSignificand >> 3 & significandMask;
    
    // Insert the exponent and sign.
    result |= (rep_t)aExponent << significandBits;
    result |= resultSign;
    
    return result;
}
