#include "GSH_VulkanMemoryUtils.h"

using namespace GSH_Vulkan;

#define MEMORY_SIZE 1024

Nuanceur::CIntRvalue CMemoryUtils::GetPixelAddress_PSMT4(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue swizzleTable, 
                                                         Nuanceur::CIntValue bufAddress, Nuanceur::CIntValue bufWidth, Nuanceur::CInt2Value position)
{
	auto pageWidth = NewInt(b, CGsPixelFormats::STORAGEPSMT4::PAGEWIDTH);
	auto pageHeight = NewInt(b, CGsPixelFormats::STORAGEPSMT4::PAGEHEIGHT);
	auto pageSize = NewInt(b, CGsPixelFormats::PAGESIZE);
	auto pageNum = (position->x() / pageWidth) + ((position->y() / pageHeight) * bufWidth) / pageWidth;
	auto pagePos = NewInt2(position->x() % pageWidth, position->y() % pageHeight);
	auto pageOffset = ToInt(Load(swizzleTable, pagePos)->x());
	return ((bufAddress + (pageNum * pageSize)) * NewInt(b, 2)) + pageOffset;
}

Nuanceur::CUintRvalue CMemoryUtils::Memory_Read32(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue memoryImage, Nuanceur::CIntValue address)
{
	auto wordAddress = address / NewInt(b, 4);
	auto position = NewInt2(wordAddress % NewInt(b, MEMORY_SIZE), wordAddress / NewInt(b, MEMORY_SIZE));
	return Load(memoryImage, position)->x();
}

Nuanceur::CUintRvalue CMemoryUtils::Memory_Read16(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue memoryImage, Nuanceur::CIntValue address)
{
	auto wordAddress = address / NewInt(b, 4);
	auto shiftAmount = (ToUint(address) & NewUint(b, 2)) * NewUint(b, 8);
	auto position = NewInt2(wordAddress % NewInt(b, MEMORY_SIZE), wordAddress / NewInt(b, MEMORY_SIZE));
	auto pixel = Load(memoryImage, position)->x();
	return (pixel >> shiftAmount) & NewUint(b, 0xFFFF);
}

Nuanceur::CUintRvalue CMemoryUtils::Memory_Read8(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue memoryImage, Nuanceur::CIntValue address)
{
	auto wordAddress = address / NewInt(b, 4);
	auto shiftAmount = (ToUint(address) & NewUint(b, 3)) * NewUint(b, 8);
	auto position = NewInt2(wordAddress % NewInt(b, MEMORY_SIZE), wordAddress / NewInt(b, MEMORY_SIZE));
	auto pixel = Load(memoryImage, position)->x();
	return (pixel >> shiftAmount) & NewUint(b, 0xFF);
}

Nuanceur::CUintRvalue CMemoryUtils::Memory_Read4(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue memoryImage, Nuanceur::CIntValue nibAddress)
{
	auto address = nibAddress / NewInt(b, 2);
	auto wordAddress = address / NewInt(b, 4);
	auto nibIndex = ToUint(nibAddress & NewInt(b, 1));
	auto shiftAmount = (((ToUint(address) & NewUint(b, 3)) * NewUint(b, 2)) + nibIndex) * NewUint(b, 4);
	auto position = NewInt2(wordAddress % NewInt(b, MEMORY_SIZE), wordAddress / NewInt(b, MEMORY_SIZE));
	auto pixel = Load(memoryImage, position)->x();
	return (pixel >> shiftAmount) & NewUint(b, 0xF);
}

void CMemoryUtils::Memory_Write32(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue memoryImage, Nuanceur::CIntValue address, Nuanceur::CUintValue value)
{
	auto wordAddress = address / NewInt(b, 4);
	auto position = NewInt2(wordAddress % NewInt(b, MEMORY_SIZE), wordAddress / NewInt(b, MEMORY_SIZE));
	Store(memoryImage, position, NewUint4(value, NewUint3(b, 0, 0, 0)));
}

void CMemoryUtils::Memory_Write16(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue memoryImage, Nuanceur::CIntValue address, Nuanceur::CUintValue value)
{
	auto wordAddress = address / NewInt(b, 4);
	auto shiftAmount = (ToUint(address) & NewUint(b, 2)) * NewUint(b, 8);
	auto mask = NewUint(b, 0xFFFFFFFF) ^ (NewUint(b, 0xFFFF) << shiftAmount);
	auto valueWord = value << shiftAmount;
	auto position = NewInt2(wordAddress % NewInt(b, MEMORY_SIZE), wordAddress / NewInt(b, MEMORY_SIZE));
	AtomicAnd(memoryImage, position, mask);
	AtomicOr(memoryImage, position, valueWord);
}

void CMemoryUtils::Memory_Write8(Nuanceur::CShaderBuilder& b, Nuanceur::CImageUint2DValue memoryImage, Nuanceur::CIntValue address, Nuanceur::CUintValue value)
{
	auto wordAddress = address / NewInt(b, 4);
	auto shiftAmount = (ToUint(address) & NewUint(b, 3)) * NewUint(b, 8);
	auto mask = NewUint(b, 0xFFFFFFFF) ^ (NewUint(b, 0xFF) << shiftAmount);
	auto valueWord = value << shiftAmount;
	auto position = NewInt2(wordAddress % NewInt(b, MEMORY_SIZE), wordAddress / NewInt(b, MEMORY_SIZE));
	AtomicAnd(memoryImage, position, mask);
	AtomicOr(memoryImage, position, valueWord);
}

Nuanceur::CFloat4Rvalue CMemoryUtils::PSM32ToVec4(Nuanceur::CShaderBuilder& b, Nuanceur::CUintValue inputColor)
{
	auto colorR = (inputColor >> NewUint(b, 0)) & NewUint(b, 0xFF);
	auto colorG = (inputColor >> NewUint(b, 8)) & NewUint(b, 0xFF);
	auto colorB = (inputColor >> NewUint(b, 16)) & NewUint(b, 0xFF);
	auto colorA = (inputColor >> NewUint(b, 24)) & NewUint(b, 0xFF);

	auto colorFloatR = ToFloat(colorR) / NewFloat(b, 255.f);
	auto colorFloatG = ToFloat(colorG) / NewFloat(b, 255.f);
	auto colorFloatB = ToFloat(colorB) / NewFloat(b, 255.f);
	auto colorFloatA = ToFloat(colorA) / NewFloat(b, 255.f);

	return NewFloat4(colorFloatR, colorFloatG, colorFloatB, colorFloatA);
}

Nuanceur::CFloat4Rvalue CMemoryUtils::PSM16ToVec4(Nuanceur::CShaderBuilder& b, Nuanceur::CUintValue inputColor)
{
	auto colorR = (inputColor >> NewUint(b, 0)) & NewUint(b, 0x1F);
	auto colorG = (inputColor >> NewUint(b, 5)) & NewUint(b, 0x1F);
	auto colorB = (inputColor >> NewUint(b, 10)) & NewUint(b, 0x1F);
	auto colorA = (inputColor >> NewUint(b, 15)) & NewUint(b, 0x1);

	auto colorFloatR = ToFloat(colorR) / NewFloat(b, 31.f);
	auto colorFloatG = ToFloat(colorG) / NewFloat(b, 31.f);
	auto colorFloatB = ToFloat(colorB) / NewFloat(b, 31.f);
	auto colorFloatA = ToFloat(colorA);

	return NewFloat4(colorFloatR, colorFloatG, colorFloatB, colorFloatA);
}

Nuanceur::CUintRvalue CMemoryUtils::Vec4ToPSM32(Nuanceur::CShaderBuilder& b, Nuanceur::CFloat4Value inputColor)
{
	auto colorR = ToUint(inputColor->x() * NewFloat(b, 255.f)) << NewUint(b, 0);
	auto colorG = ToUint(inputColor->y() * NewFloat(b, 255.f)) << NewUint(b, 8);
	auto colorB = ToUint(inputColor->z() * NewFloat(b, 255.f)) << NewUint(b, 16);
	auto colorA = ToUint(inputColor->w() * NewFloat(b, 255.f)) << NewUint(b, 24);
	return colorR | colorG | colorB | colorA;
}

Nuanceur::CUintRvalue CMemoryUtils::Vec4ToPSM16(Nuanceur::CShaderBuilder& b, Nuanceur::CFloat4Value inputColor)
{
	auto colorR = ToUint(inputColor->x() * NewFloat(b, 31.f)) << NewUint(b, 0);
	auto colorG = ToUint(inputColor->y() * NewFloat(b, 31.f)) << NewUint(b, 5);
	auto colorB = ToUint(inputColor->z() * NewFloat(b, 31.f)) << NewUint(b, 10);
	auto colorA = ToUint(inputColor->w()) << NewUint(b, 15);
	return colorR | colorG | colorB | colorA;
}
