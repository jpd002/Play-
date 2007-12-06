#ifndef _CODEGEN_STACKPATTERNS_H_
#define _CODEGEN_STACKPATTERNS_H_

typedef CArrayStack<uint32> ShadowStack;

struct INTEGER64
{
    union
    {
        uint64 q;
        struct
        {
            uint32 d0;
            uint32 d1;
        };
        uint32 d[2];
    };
};

template <CCodeGen::SYMBOLS Symbol>
struct GenericOneArgument
{
    typedef uint32 PatternValue;

    bool Fits(const ShadowStack& shadowStack) const
    {
        return shadowStack.GetAt(0) == Symbol;
    }

    PatternValue Get(ShadowStack& shadowStack)
    {
        if(shadowStack.Pull() != Symbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
        return shadowStack.Pull();
    }
};

template <CCodeGen::SYMBOLS FirstSymbol, CCodeGen::SYMBOLS SecondSymbol>
struct GenericTwoArguments
{
    typedef std::pair<uint32, uint32> PatternValue;

    bool Fits(const ShadowStack& shadowStack) const
    {
        return (
            (shadowStack.GetAt(0) == SecondSymbol) &&
            (shadowStack.GetAt(2) == FirstSymbol)
            );
    }

    PatternValue Get(ShadowStack& shadowStack) const
    {
        PatternValue patternValue;
        if(shadowStack.Pull() != SecondSymbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
        patternValue.second = shadowStack.Pull();
        if(shadowStack.Pull() != FirstSymbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
        patternValue.first = shadowStack.Pull();
        return patternValue;
    }
};

template <CCodeGen::SYMBOLS FirstSymbol, CCodeGen::SYMBOLS SecondSymbol>
struct GenericCommutative
{
    typedef std::pair<uint32, uint32> PatternValue;

    bool Fits(const ShadowStack& shadowStack) const
    {
        return (
            (shadowStack.GetAt(0) == FirstSymbol) &&
            (shadowStack.GetAt(2) == SecondSymbol)
            ) || (
            (shadowStack.GetAt(0) == SecondSymbol) &&
            (shadowStack.GetAt(2) == FirstSymbol)
            );
    }

    PatternValue Get(ShadowStack& shadowStack) const
    {
        PatternValue patternValue;
        for(unsigned int i = 0; i < 2; i++)
        {
            uint32 type = shadowStack.Pull();
            uint32 value = shadowStack.Pull();
            if(type == FirstSymbol)
            {
                patternValue.first = value;
            }
            else if(type == SecondSymbol)
            {
                patternValue.second = value;
            }
            else
            {
                throw std::runtime_error("Stack consistency error.");
            }
        }
        return patternValue;
    }
};

template <CCodeGen::SYMBOLS Symbol>
struct GenericOneArgument64
{
    typedef INTEGER64 PatternValue;

    bool Fits(const ShadowStack& shadowStack) const
    {
        return 
            shadowStack.GetAt(0) == Symbol && 
            shadowStack.GetAt(2) == Symbol;
    }

    PatternValue Get(ShadowStack& shadowStack) const
    {
        PatternValue patternValue;
        for(int i = 1; i >= 0; i--)
        {
            uint32 type = shadowStack.Pull();
            uint32 value = shadowStack.Pull();
            if(type != Symbol)
            {
                throw std::runtime_error("Stack consistency error.");
            }
            patternValue.d[i] = value;
        }
        return patternValue;
    }
};

template <CCodeGen::SYMBOLS FirstSymbol, CCodeGen::SYMBOLS SecondSymbol>
struct GenericTwoArguments64
{
    typedef std::pair<INTEGER64, INTEGER64> PatternValue;

    bool Fits(const ShadowStack& shadowStack) const
    {
	    return 
            (shadowStack.GetAt(0) == SecondSymbol) && 
            (shadowStack.GetAt(2) == SecondSymbol) && 
            (shadowStack.GetAt(4) == FirstSymbol) && 
            (shadowStack.GetAt(6) == FirstSymbol);
    }

    PatternValue Get(ShadowStack& shadowStack) const
    {
        PatternValue value;
        if(shadowStack.Pull() != SecondSymbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.second.d1 = shadowStack.Pull();
        if(shadowStack.Pull() != SecondSymbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.second.d0 = shadowStack.Pull();
        if(shadowStack.Pull() != FirstSymbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.first.d1 = shadowStack.Pull();
        if(shadowStack.Pull() != FirstSymbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.first.d0 = shadowStack.Pull();
        return value;
    }
};

template <CCodeGen::SYMBOLS FirstSymbol, CCodeGen::SYMBOLS SecondSymbol>
struct GenericCommutative64
{
    typedef std::pair<INTEGER64, INTEGER64> PatternValue;

    bool Fits(const ShadowStack& shadowStack) const
    {
        return 
        (
            (shadowStack.GetAt(0) == FirstSymbol) &&
            (shadowStack.GetAt(2) == FirstSymbol) &&
            (shadowStack.GetAt(4) == SecondSymbol) &&
            (shadowStack.GetAt(6) == SecondSymbol)
        ) || (
            (shadowStack.GetAt(0) == SecondSymbol) &&
            (shadowStack.GetAt(2) == SecondSymbol) &&
            (shadowStack.GetAt(4) == FirstSymbol) &&
            (shadowStack.GetAt(6) == FirstSymbol)
        );
    }

    PatternValue Get(ShadowStack& shadowStack) const
    {
        PatternValue patternValue;

        for(unsigned int i = 0; i < 2; i++)
        {
            uint32 type[2];
            uint32 value[2];
            for(unsigned int j = 0; j < 2; j++)
            {
                type[j] = shadowStack.Pull();
                value[j] = shadowStack.Pull();
            }
            if(type[0] == FirstSymbol && type[1] == FirstSymbol)
            {
                patternValue.first.d1 = value[0];
                patternValue.first.d0 = value[1];
            }
            else if(type[0] == SecondSymbol && type[1] == SecondSymbol)
            {
                patternValue.second.d1 = value[0];
                patternValue.second.d0 = value[1];
            }
            else
            {
                throw std::runtime_error("Stack consistency error.");
            }
        }

        return patternValue;
    }
};
/*
struct RelativeRelative64ConstantHighOrder
{
    enum 
    {
        Symbol = CCodeGen::RELATIVE,
    };

    typedef std::pair<INTEGER64, INTEGER64> PatternValue;

    bool Fits(const ShadowStack& shadowStack) const
    {
        return 
            shadowStack.GetAt(0) == Symbol &&
            shadowStack.GetAt(2) == CCodeGen::CONSTANT &&
            shadowStack.GetAt(4) == Symbol &&
            shadowStack.GetAt(6) == CCodeGen::CONSTANT;
    }

    PatternValue Get(ShadowStack& shadowStack) const
    {
        PatternValue value;
        if(shadowStack.Pull() != CCodeGen::CONSTANT)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.second.d1 = shadowStack.Pull();
        if(shadowStack.Pull() != Symbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.second.d0 = shadowStack.Pull();
        if(shadowStack.Pull() != CCodeGen::CONSTANT)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.first.d1 = shadowStack.Pull();
        if(shadowStack.Pull() != Symbol)
        {
            throw std::runtime_error("Stack consistency error.");
        }
	    value.first.d0 = shadowStack.Pull();
        return value;        
    }
};
*/
typedef GenericOneArgument<CCodeGen::RELATIVE> SingleRelative;
typedef GenericOneArgument<CCodeGen::REGISTER> SingleRegister;
typedef GenericOneArgument<CCodeGen::CONSTANT> SingleConstant;
typedef GenericTwoArguments<CCodeGen::RELATIVE, CCodeGen::CONSTANT> RelativeConstant;
typedef GenericTwoArguments<CCodeGen::REGISTER, CCodeGen::CONSTANT> RegisterConstant;
typedef GenericTwoArguments<CCodeGen::CONSTANT, CCodeGen::RELATIVE> ConstantRelative;
typedef GenericTwoArguments<CCodeGen::CONSTANT, CCodeGen::CONSTANT> ConstantConstant;
typedef GenericTwoArguments<CCodeGen::RELATIVE, CCodeGen::RELATIVE> RelativeRelative;
typedef GenericCommutative<CCodeGen::REGISTER, CCodeGen::CONSTANT> CommutativeRegisterConstant;
typedef GenericCommutative<CCodeGen::RELATIVE, CCodeGen::CONSTANT> CommutativeRelativeConstant;
typedef GenericOneArgument64<CCodeGen::CONSTANT> SingleConstant64;
typedef GenericTwoArguments64<CCodeGen::CONSTANT, CCodeGen::CONSTANT> ConstantConstant64;
typedef GenericCommutative64<CCodeGen::RELATIVE, CCodeGen::CONSTANT> CommutativeRelativeConstant64;

#endif
