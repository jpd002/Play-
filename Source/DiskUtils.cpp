#include <algorithm>
#include <cassert>
#include <cstring>
#include "string_cast.h"
#include "stricmp.h"
#include "DiskUtils.h"
#include "discimages/ChdCdImageStream.h"
#include "discimages/CsoImageStream.h"
#include "discimages/CueSheet.h"
#include "discimages/IszImageStream.h"
#include "discimages/MdsDiscImage.h"
#include "StdStream.h"
#include "StdStreamUtils.h"
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
#include "android/ContentStream.h"
#include "android/ContentUtils.h"
#endif
#ifdef __EMSCRIPTEN__
#include "Js_DiscImageDeviceStream.h"
#endif
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

static std::unique_ptr<Framework::CStream> CreateImageStream(const fs::path& imagePath)
{
	static const auto s3ImagePathPrefix = fs::path("//s3/").native();
	auto imagePathString = imagePath.native();
	if(imagePathString.find(s3ImagePathPrefix) == 0)
	{
#ifdef HAS_AMAZON_S3
		//TODO: Remove string_cast here and support unicode characters.
		auto fullObjectPath = string_cast<std::string>(imagePathString.substr(s3ImagePathPrefix.length()));
		std::replace(fullObjectPath.begin(), fullObjectPath.end(), '\\', '/');
		auto objectPathPos = fullObjectPath.find('/');
		if(objectPathPos == std::string::npos)
		{
			throw std::runtime_error("Invalid S3 object path.");
		}
		auto bucketName = std::string(fullObjectPath.begin(), fullObjectPath.begin() + objectPathPos);
		return std::make_unique<CS3ObjectStream>(bucketName.c_str(), fullObjectPath.c_str() + objectPathPos + 1);
#else
		throw std::runtime_error("S3 support was disabled during build configuration.");
#endif
	}
#ifdef __ANDROID__
	if(Framework::Android::CContentUtils::IsContentPath(imagePath))
	{
		auto uri = Framework::Android::CContentUtils::BuildUriFromPath(imagePath);
		return std::make_unique<Framework::Android::CContentStream>(uri.c_str(), "r");
	}
	else
	{
		return std::make_unique<Framework::CPosixFileStream>(imagePathString.c_str(), O_RDONLY);
	}
#elif defined(__EMSCRIPTEN__)
	return std::make_unique<CJsDiscImageDeviceStream>();
#else
	return std::make_unique<Framework::CStdStream>(imagePathString.c_str(), Framework::GetInputStdStreamMode<fs::path::string_type>());
#endif
}

static DiskUtils::OpticalMediaPtr CreateOpticalMediaFromCueSheet(const fs::path& imagePath)
{
	auto currentPath = imagePath.parent_path();
	auto imageStream = std::unique_ptr<Framework::CStream>(CreateImageStream(imagePath));
	auto fileStream = std::shared_ptr<Framework::CStream>();
	CCueSheet cueSheet(*imageStream);
	for(const auto& command : cueSheet.GetCommands())
	{
		if(auto fileCommand = dynamic_cast<CCueSheet::COMMAND_FILE*>(command.get()))
		{
			assert(fileCommand->filetype == "BINARY");
			auto filePath = currentPath / fileCommand->filename;
			fileStream = std::shared_ptr<Framework::CStream>(CreateImageStream(filePath));
			break;
		}
	}
	if(!fileStream)
	{
		throw std::runtime_error("Could not build media from cuesheet.");
	}
	return COpticalMedia::CreateAuto(fileStream);
}

static DiskUtils::OpticalMediaPtr CreateOpticalMediaFromMds(const fs::path& imagePath)
{
	auto imageStream = std::unique_ptr<Framework::CStream>(CreateImageStream(imagePath));
	auto discImage = CMdsDiscImage(*imageStream);

	//Create image data path
	auto imageDataPath = imagePath;
	imageDataPath.replace_extension("mdf");
	auto imageDataStream = std::shared_ptr<Framework::CStream>(CreateImageStream(imageDataPath));

	return COpticalMedia::CreateDvd(imageDataStream, discImage.IsDualLayer(), discImage.GetLayerBreak());
}

static DiskUtils::OpticalMediaPtr CreateOpticalMediaFromChd(const fs::path& imagePath)
{
	//Some notes about CHD support:
	//- We don't support multi track CDs
	auto imageStream = std::make_shared<CChdCdImageStream>(CreateImageStream(imagePath));
	auto trackInfo = [&imageStream]() -> std::pair<COpticalMedia::BlockProviderPtr, COpticalMedia::TRACK_DATA_TYPE> {
		switch(imageStream->GetTrack0Type())
		{
		default:
			assert(false);
			[[fallthrough]];
		case CChdCdImageStream::TRACK_TYPE_CD_MODE1:
			return std::make_pair(std::make_shared<ISO9660::CBlockProviderCustom<0x990, 0>>(imageStream), COpticalMedia::TRACK_DATA_TYPE_MODE1_2048);
		case CChdCdImageStream::TRACK_TYPE_CD_MODE2_RAW:
			return std::make_pair(std::make_shared<ISO9660::CBlockProviderCustom<0x990, 0x18>>(imageStream), COpticalMedia::TRACK_DATA_TYPE_MODE2_2352);
		case CChdCdImageStream::TRACK_TYPE_DVD:
			return std::make_pair(std::make_shared<ISO9660::CBlockProvider2048>(imageStream), COpticalMedia::TRACK_DATA_TYPE_MODE1_2048);
		}
	}();
	return COpticalMedia::CreateCustomSingleTrack(std::move(trackInfo.first), trackInfo.second);
}

const DiskUtils::ExtensionList& DiskUtils::GetSupportedExtensions()
{
	static auto extensionList = ExtensionList{".iso", ".mds", ".isz", ".cso", ".cue", ".chd"};
	return extensionList;
}

DiskUtils::OpticalMediaPtr DiskUtils::CreateOpticalMediaFromPath(const fs::path& imagePath, uint32 opticalMediaCreateFlags)
{
	assert(!imagePath.empty());

	std::shared_ptr<Framework::CStream> stream;
	auto extension = imagePath.extension().string();

	//Gotta think of something better than that...
	if(!stricmp(extension.c_str(), ".isz"))
	{
		stream = std::make_shared<CIszImageStream>(CreateImageStream(imagePath));
	}
	else if(!stricmp(extension.c_str(), ".chd"))
	{
		return CreateOpticalMediaFromChd(imagePath);
	}
	else if(!stricmp(extension.c_str(), ".cso"))
	{
		stream = std::make_shared<CCsoImageStream>(CreateImageStream(imagePath));
	}
	else if(!stricmp(extension.c_str(), ".cue"))
	{
		return CreateOpticalMediaFromCueSheet(imagePath);
	}
	else if(!stricmp(extension.c_str(), ".mds"))
	{
		return CreateOpticalMediaFromMds(imagePath);
	}
#ifdef _WIN32
	else if(imagePath.string()[0] == '\\')
	{
		stream = std::make_shared<Framework::Win32::CVolumeStream>(imagePath.native().c_str());
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

	return COpticalMedia::CreateAuto(stream, opticalMediaCreateFlags);
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

bool DiskUtils::TryGetDiskId(const fs::path& imagePath, std::string* diskIdPtr)
{
	try
	{
		auto opticalMedia = CreateOpticalMediaFromPath(imagePath, COpticalMedia::CREATE_AUTO_DISABLE_DL_DETECT);
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
