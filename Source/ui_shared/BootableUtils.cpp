#include <algorithm>
#include <cstring>
#include "BootableUtils.h"
#include "DiskUtils.h"

bool BootableUtils::IsBootableExecutablePath(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".elf");
}

bool BootableUtils::IsBootableDiscImagePath(const fs::path& filePath)
{
	const auto& supportedExtensions = DiskUtils::GetSupportedExtensions();
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	auto extensionIterator = supportedExtensions.find(extension);
	return extensionIterator != std::end(supportedExtensions);
}

bool BootableUtils::IsBootableArcadeDefPath(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	return (extension == ".arcadedef");
}

BootableUtils::BOOTABLE_TYPE BootableUtils::GetBootableType(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if(extension == ".elf")
		return BootableUtils::BOOTABLE_TYPE::PS2_ELF;
	if(extension == ".arcadedef")
		return BootableUtils::BOOTABLE_TYPE::PS2_ARCADE;

	if(BootableUtils::IsBootableDiscImagePath(filePath))
	{
		try
		{
			auto opticalMedia = DiskUtils::CreateOpticalMediaFromPath(filePath, COpticalMedia::CREATE_AUTO_DISABLE_DL_DETECT);
			auto fileSystem = opticalMedia->GetFileSystem();
			auto systemConfigFile = std::unique_ptr<Framework::CStream>(fileSystem->Open("SYSTEM.CNF;1"));
			if(!systemConfigFile) return BootableUtils::BOOTABLE_TYPE::UNKNOWN;

			auto systemConfig = DiskUtils::ParseSystemConfigFile(systemConfigFile.get());

			if(auto bootItemIterator = systemConfig.find("BOOT2"); bootItemIterator != std::end(systemConfig))
			{
				return BootableUtils::BOOTABLE_TYPE::PS2_DISC;
			}
		}
		catch(const std::exception&)
		{
		}
	}
	return BootableUtils::BOOTABLE_TYPE::UNKNOWN;
}
