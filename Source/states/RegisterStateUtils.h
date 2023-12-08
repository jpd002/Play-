#pragma once

#include "string_format.h"

namespace RegisterStateUtils
{
	template <typename RegisterStateType, typename ArrayType, std::size_t ArraySize>
	void ReadArray(const RegisterStateType& registerState, ArrayType (&array)[ArraySize], const char* format)
	{
		static_assert((sizeof(array) & 0x0F) == 0);
		static const size_t regCount = sizeof(array) / (sizeof(uint128));
		for(size_t i = 0; i < regCount; i++)
		{
			auto registerName = string_format(format, i);
			reinterpret_cast<uint128*>(array)[i] = registerState.GetRegister128(registerName.c_str());
		}
	}

	template <typename RegisterStateType, typename ArrayType, std::size_t ArraySize>
	void WriteArray(RegisterStateType& registerState, const ArrayType (&array)[ArraySize], const char* format)
	{
		static_assert((sizeof(array) & 0x0F) == 0);
		static const size_t regCount = sizeof(array) / (sizeof(uint128));
		for(size_t i = 0; i < regCount; i++)
		{
			auto registerName = string_format(format, i);
			registerState.SetRegister128(registerName.c_str(), reinterpret_cast<const uint128*>(array)[i]);
		}
	}
}
