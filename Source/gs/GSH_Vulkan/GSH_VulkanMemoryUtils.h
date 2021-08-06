#pragma once

#include "nuanceur/Builder.h"
#include "../GsPixelFormats.h"

namespace GSH_Vulkan
{
	class CMemoryUtils
	{
	public:
		template <typename StorageFormat>
		static Nuanceur::CIntRvalue GetPixelAddress(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue swizzleTable,
		                                            Nuanceur::CIntValue bufAddress, Nuanceur::CIntValue bufWidth, Nuanceur::CInt2Value position)
		{
			auto pageWidth = NewInt(b, StorageFormat::PAGEWIDTH);
			auto pageHeight = NewInt(b, StorageFormat::PAGEHEIGHT);
			auto pageSize = NewInt(b, CGsPixelFormats::PAGESIZE);
			auto pageNum = (position->x() / pageWidth) + ((position->y() / pageHeight) * bufWidth) / pageWidth;
			auto pagePos = NewInt2(position->x() % pageWidth, position->y() % pageHeight);
			auto pageOffset = ToInt(Load(swizzleTable, pagePos)->x());
			return bufAddress + (pageNum * pageSize) + pageOffset;
		}

		static Nuanceur::CIntRvalue GetPixelAddress_PSMT4(Nuanceur::CShaderBuilder&, Nuanceur::CImageUint2DValue, Nuanceur::CIntValue, Nuanceur::CIntValue, Nuanceur::CInt2Value);

		static Nuanceur::CUintRvalue Memory_Read32(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue);
		static Nuanceur::CUintRvalue Memory_Read24(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue);
		static Nuanceur::CUintRvalue Memory_Read16(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue);
		static Nuanceur::CUintRvalue Memory_Read8(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue);
		static Nuanceur::CUintRvalue Memory_Read4(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue);

		static void Memory_Write32(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CUintValue);
		static void Memory_Write24(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CUintValue);
		static void Memory_Write24(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUcharValue, Nuanceur::CArrayUshortValue, Nuanceur::CIntValue, Nuanceur::CUintValue);
		static void Memory_Write16(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CUintValue);
		static void Memory_Write16(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUshortValue, Nuanceur::CIntValue, Nuanceur::CUintValue);
		static void Memory_Write8(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CUintValue);
		static void Memory_Write8(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUcharValue, Nuanceur::CIntValue, Nuanceur::CUintValue);
		static void Memory_Write4(Nuanceur::CShaderBuilder&, Nuanceur::CArrayUintValue, Nuanceur::CIntValue, Nuanceur::CUintValue);

		static Nuanceur::CFloat4Rvalue PSM32ToVec4(Nuanceur::CShaderBuilder&, Nuanceur::CUintValue);
		static Nuanceur::CFloat4Rvalue PSM16ToVec4(Nuanceur::CShaderBuilder&, Nuanceur::CUintValue);

		static Nuanceur::CUintRvalue Vec4ToPSM32(Nuanceur::CShaderBuilder&, Nuanceur::CFloat4Value);
		static Nuanceur::CUintRvalue Vec4ToPSM16(Nuanceur::CShaderBuilder&, Nuanceur::CFloat4Value);

		static Nuanceur::CInt4Rvalue PSM32ToIVec4(Nuanceur::CShaderBuilder&, Nuanceur::CUintValue);
		static Nuanceur::CInt4Rvalue PSM16ToIVec4(Nuanceur::CShaderBuilder&, Nuanceur::CUintValue);

		static Nuanceur::CUintRvalue IVec4ToPSM32(Nuanceur::CShaderBuilder&, Nuanceur::CInt4Value);
		static Nuanceur::CUintRvalue IVec4ToPSM16(Nuanceur::CShaderBuilder&, Nuanceur::CInt4Value);
	};
}
