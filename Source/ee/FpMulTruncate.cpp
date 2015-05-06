//Source: The LLVM Compiler Infrastructure - lib/mulsf3.c
//Modified to truncate result of multiplication

#include <limits.h>
#include <boost/cstdint.hpp>
#include "FpMulTruncate.h"

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

static rep_t toRep(fp_t x) {
    union { fp_t f; rep_t i; } rep;
	rep.f = x;
    return rep.i;
}

static fp_t fromRep(rep_t x) {
    union { fp_t f; rep_t i; } rep;
	rep.i = x;
    return rep.f;
}

// 32x32 --> 64 bit multiply
static void wideMultiply(rep_t a, rep_t b, rep_t *hi, rep_t *lo) {
    const uint64 product = (uint64)a*b;
    *hi = product >> 32;
    *lo = product;
}

static void wideLeftShift(rep_t *hi, rep_t *lo, int count) {
    *hi = *hi << count | *lo >> (typeWidth - count);
    *lo = *lo << count;
}

static void wideRightShiftWithSticky(rep_t *hi, rep_t *lo, int count) {
    if (count < typeWidth) {
        const bool sticky = *lo << (typeWidth - count);
        *lo = *hi << (typeWidth - count) | *lo >> count | sticky;
        *hi = *hi >> count;
    }
    else if (count < 2*typeWidth) {
        const bool sticky = *hi << (2*typeWidth - count) | *lo;
        *lo = *hi >> (count - typeWidth) | sticky;
        *hi = 0;
    } else {
        const bool sticky = *hi | *lo;
        *lo = sticky;
        *hi = 0;
    }
}

uint32 FpMulTruncate(uint32 a, uint32 b) 
{
	const unsigned int aExponent = a >> significandBits & maxExponent;
	const unsigned int bExponent = b >> significandBits & maxExponent;
	const rep_t productSign = (a ^ b) & signBit;
    
    rep_t aSignificand = a & significandMask;
    rep_t bSignificand = b & significandMask;
    int scale = 0;
    
    // Detect if a or b is zero, denormal, infinity, or NaN.
    if (aExponent-1U >= maxExponent-1U || bExponent-1U >= maxExponent-1U) {
        
        const rep_t aAbs = a & absMask;
        const rep_t bAbs = b & absMask;
        
        // NaN * anything = qNaN
        if (aAbs > infRep) return (a | quietBit);
        // anything * NaN = qNaN
        if (bAbs > infRep) return (b | quietBit);
        
        if (aAbs == infRep) {
            // infinity * non-zero = +/- infinity
            if (bAbs) return (aAbs | productSign);
            // infinity * zero = NaN
            else return qnanRep;
        }
        
        if (bAbs == infRep) {
            // non-zero * infinity = +/- infinity
            if (aAbs) return (bAbs | productSign);
            // zero * infinity = NaN
            else return qnanRep;
        }
        
        // zero * anything = +/- zero
        if (!aAbs) return productSign;
        // anything * zero = +/- zero
        if (!bAbs) return productSign;
        
        // one or both of a or b is denormal, the other (if applicable) is a
        // normal number.  Renormalize one or both of a and b, and set scale to
        // include the necessary exponent adjustment.
        if (aAbs < implicitBit) aSignificand = 0;
        if (bAbs < implicitBit) bSignificand = 0;
    }
    
    // Or in the implicit significand bit.  (If we fell through from the
    // denormal path it was already set by normalize( ), but setting it twice
    // won't hurt anything.)
    aSignificand |= implicitBit;
    bSignificand |= implicitBit;
    
    // Get the significand of a*b.  Before multiplying the significands, shift
    // one of them left to left-align it in the field.  Thus, the product will
    // have (exponentBits + 2) integral digits, all but two of which must be
    // zero.  Normalizing this result is just a conditional left-shift by one
    // and bumping the exponent accordingly.
    rep_t productHi, productLo;
    wideMultiply(aSignificand, bSignificand << exponentBits,
                 &productHi, &productLo);
    
    int productExponent = aExponent + bExponent - exponentBias + scale;
    
    // Normalize the significand, adjust exponent if needed.
    if (productHi & implicitBit)
	{
		productExponent++;
	}
    else
	{
		wideLeftShift(&productHi, &productLo, 1);
	}
    
    // If we have overflowed the type, return +/- infinity.
    if (productExponent >= maxExponent) return infRep | productSign;
    
    if (productExponent <= 0) {
        // Result is denormal before rounding, the exponent is zero and we
        // need to shift the significand.
        wideRightShiftWithSticky(&productHi, &productLo, 1 - productExponent);
    }
    else 
	{
        // Result is normal before rounding; insert the exponent.
        productHi &= significandMask;
        productHi |= (rep_t)productExponent << significandBits;
    }
    
    // Insert the sign of the result:
    productHi |= productSign;
    
    return productHi;
}
