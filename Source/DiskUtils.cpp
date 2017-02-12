#include <boost/algorithm/string.hpp>
#include "make_unique.h"
#include "Utils.h"
#include "stricmp.h"
#include "DiskUtils.h"
#include "IszImageStream.h"
#include "CsoImageStream.h"
#include "StdStream.h"
#ifdef WIN32
#include "VolumeStream.h"
#else
#include "Posix_VolumeStream.h"
#endif
#ifdef __ANDROID__
#include "PosixFileStream.h"
#endif
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

static Framework::CStream* CreateImageStream(const boost::filesystem::path& imagePath)
{
#ifdef __ANDROID__
	return new Framework::CPosixFileStream(imagePath.string().c_str(), O_RDONLY);
#else
	return new Framework::CStdStream(imagePath.string().c_str(), "rb");
#endif
}

DiskUtils::Iso9660Ptr DiskUtils::CreateDiskImageFromPath(const boost::filesystem::path& imagePath)
{
	assert(!imagePath.empty());

	std::shared_ptr<Framework::CStream> stream;
	auto extension = imagePath.extension().string();

	//Gotta think of something better than that...
	if(!stricmp(extension.c_str(), ".isz"))
	{
		stream = std::make_shared<CIszImageStream>(CreateImageStream(imagePath));
	}
	else if(!stricmp(extension.c_str(), ".cso"))
	{
		stream = std::make_shared<CCsoImageStream>(CreateImageStream(imagePath));
	}
#ifdef WIN32
	else if(imagePath.string()[0] == '\\')
	{
		stream = std::make_shared<Framework::Win32::CVolumeStream>(imagePath.string()[4]);
	}
#elif !defined(__ANDROID__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
	else
	{
		try
		{
			stream = std::make_shared<Framework::Posix::CVolumeStream>(imagePath.string().c_str());
		}
		catch(...)
		{
			//Ok if it fails here, might be a standard ISO image file
			//which will be handled below
		}
	}
#endif

	//If it's null after all that, just feed it to a StdStream
	if(!stream)
	{
		stream = std::shared_ptr<Framework::CStream>(CreateImageStream(imagePath));
	}

	Iso9660Ptr result;
	try
	{
		auto blockProvider = std::make_shared<ISO9660::CBlockProvider2048>(stream);
		result = std::make_unique<CISO9660>(blockProvider);
	}
	catch(...)
	{
		//Failed with block size 2048, try with CD-ROM XA
		auto blockProvider = std::make_shared<ISO9660::CBlockProviderCDROMXA>(stream);
		result = std::make_unique<CISO9660>(blockProvider);
	}
	return result;
}

DiskUtils::SystemConfigMap DiskUtils::ParseSystemConfigFile(Framework::CStream* systemCnfFile)
{
	SystemConfigMap result;
	auto line = Utils::GetLine(systemCnfFile);
	while(!systemCnfFile->IsEOF())
	{
		auto trimmedEnd = std::remove_if(line.begin(), line.end(), isspace);
		auto trimmedLine = std::string(line.begin(), trimmedEnd);
		std::vector<std::string> components;
		boost::split(components, trimmedLine, boost::is_any_of("="), boost::algorithm::token_compress_on);
		if(components.size() >= 2)
		{
			result.insert(std::make_pair(components[0], components[1]));
		}
		line = Utils::GetLine(systemCnfFile);
	}
	return result;
}

static std::string GetDiskIdFromPath(const std::string& filePath)
{
	//Expecting something like SCUS_XXX.XX;1
	if(filePath.length() < 13)
	{
		throw std::runtime_error("File name too short");
	}

	auto subFilePath = filePath.substr(filePath.length() - 13);
	auto regionCode = subFilePath.substr(0, 4);
	auto serial1 = subFilePath.substr(5, 3);
	auto serial2 = subFilePath.substr(9, 2);
	return regionCode + "_" + serial1 + "." + serial2;
}

bool DiskUtils::TryGetDiskId(const boost::filesystem::path& imagePath, std::string* diskIdPtr)
{
	try
	{
		auto diskImage = CreateDiskImageFromPath(imagePath);
		auto systemConfigFile = std::unique_ptr<Framework::CStream>(diskImage->Open("SYSTEM.CNF;1"));
		if(!systemConfigFile) return false;

		auto systemConfig = ParseSystemConfigFile(systemConfigFile.get());
		auto bootItemIterator = systemConfig.find("BOOT2");
		if(bootItemIterator == std::end(systemConfig)) return false;

		auto diskId = GetDiskIdFromPath(bootItemIterator->second);
		if(diskIdPtr)
		{
			(*diskIdPtr) = diskId;
		}
		return true;
	}
	catch(const std::exception&)
	{
		return false;
	}
}
