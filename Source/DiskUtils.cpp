#include "make_unique.h"
#include "stricmp.h"
#include "DiskUtils.h"
#include "IszImageStream.h"
#include "CsoImageStream.h"
#include "MdsDiscImage.h"
#include "StdStream.h"
#include "StringUtils.h"
#ifdef HAS_AMAZON_S3
#include "s3stream/S3ObjectStream.h"
#endif
#ifdef _WIN32
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
	auto imagePathString = imagePath.string();
	if(imagePathString.find("s3://") == 0)
	{
#ifdef HAS_AMAZON_S3
		auto fullObjectPath = std::string(imagePathString.c_str() + 5);
		auto objectPathPos = fullObjectPath.find('/');
		if(objectPathPos == std::string::npos)
		{
			throw std::runtime_error("Invalid S3 object path.");
		}
		auto bucketName = std::string(fullObjectPath.begin(), fullObjectPath.begin() + objectPathPos);
		return new CS3ObjectStream(bucketName.c_str(), fullObjectPath.c_str() + objectPathPos + 1);
#else
		throw std::runtime_error("S3 support was disabled during build configuration.");
#endif
	}
#ifdef __ANDROID__
	return new Framework::CPosixFileStream(imagePathString.c_str(), O_RDONLY);
#else
	return new Framework::CStdStream(imagePathString.c_str(), "rb");
#endif
}

DiskUtils::OpticalMediaPtr DiskUtils::CreateOpticalMediaFromPath(const boost::filesystem::path& imagePath)
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
	else if(!stricmp(extension.c_str(), ".mds"))
	{
		auto imageStream = std::unique_ptr<Framework::CStream>(CreateImageStream(imagePath));
		auto discImage = CMdsDiscImage(*imageStream);

		//Create image data path
		auto imageDataPath = imagePath;
		imageDataPath.replace_extension("mdf");
		auto imageDataStream = std::shared_ptr<Framework::CStream>(CreateImageStream(imageDataPath));

		return std::unique_ptr<COpticalMedia>(COpticalMedia::CreateDvd(imageDataStream, discImage.IsDualLayer(), discImage.GetLayerBreak()));
	}
#ifdef _WIN32
	else if(imagePath.string()[0] == '\\')
	{
		stream = std::make_shared<Framework::Win32::CVolumeStream>(imagePath.string()[4]);
	}
#elif !defined(__ANDROID__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
	else if(imagePath.string().find("/dev/") == 0)
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

	return std::unique_ptr<COpticalMedia>(COpticalMedia::CreateAuto(stream));
}

DiskUtils::SystemConfigMap DiskUtils::ParseSystemConfigFile(Framework::CStream* systemCnfFile)
{
	SystemConfigMap result;
	auto line = systemCnfFile->ReadLine();
	while(!systemCnfFile->IsEOF())
	{
		auto trimmedEnd = std::remove_if(line.begin(), line.end(), isspace);
		auto trimmedLine = std::string(line.begin(), trimmedEnd);
		std::vector<std::string> components = StringUtils::Split(trimmedLine, '=', true);
		if(components.size() >= 2)
		{
			result.insert(std::make_pair(components[0], components[1]));
		}
		line = systemCnfFile->ReadLine();
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
	return regionCode + "-" + serial1 + serial2;
}

bool DiskUtils::TryGetDiskId(const boost::filesystem::path& imagePath, std::string* diskIdPtr)
{
	try
	{
		auto opticalMedia = CreateOpticalMediaFromPath(imagePath);
		auto fileSystem = opticalMedia->GetFileSystem();
		auto systemConfigFile = std::unique_ptr<Framework::CStream>(fileSystem->Open("SYSTEM.CNF;1"));
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
