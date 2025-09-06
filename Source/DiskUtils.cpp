#include <algorithm>
#include <cassert>
#include <cstring>
#include "string_cast.h"
#include "string_format.h"
#include "stricmp.h"
#include "DiskUtils.h"
#include "discimages/ChdCdImageStream.h"
#include "discimages/CsoImageStream.h"
#include "discimages/CueSheet.h"
#include "discimages/IszImageStream.h"
#include "discimages/MdsDiscImage.h"
#include "discimages/MultiImageStream.h"
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
	CCueSheet cueSheet(*imageStream);
	struct TRACK
	{
		std::shared_ptr<Framework::CStream> stream;
		uint64 size = 0;
	};
	std::vector<TRACK> tracks;
	auto currentStream = std::shared_ptr<Framework::CStream>();
	int currentTrackIndex = -1;
	for(const auto& command : cueSheet.GetCommands())
	{
		if(auto fileCommand = dynamic_cast<CCueSheet::COMMAND_FILE*>(command.get()))
		{
			if(fileCommand->filetype != "BINARY")
			{
				throw std::runtime_error(string_format("Unsupported FILE type: %s.", fileCommand->filetype.c_str()));
			}
			auto filePath = currentPath / fileCommand->filename;
			currentStream = std::shared_ptr<Framework::CStream>(CreateImageStream(filePath));
			currentTrackIndex = -1;
		}
		else if(auto trackCommand = dynamic_cast<CCueSheet::COMMAND_TRACK*>(command.get()))
		{
			if(trackCommand->number != (tracks.size() + 1))
			{
				throw std::runtime_error("Unexpected track command encountered.");
			}
			if(currentTrackIndex != -1)
			{
				throw std::runtime_error("Multiple tracks per file not supported.");
			}
			currentTrackIndex = trackCommand->number - 1;
			TRACK newTrack = {};
			newTrack.stream = currentStream;
			tracks.push_back(std::move(newTrack));
		}
		else if(auto indexCommand = dynamic_cast<CCueSheet::COMMAND_INDEX*>(command.get()))
		{
			if(currentTrackIndex == -1)
			{
				throw std::runtime_error("Got an index without defining a track.");
			}
			if(indexCommand->number == 1)
			{
				auto& track = tracks[currentTrackIndex];
				//Store pregap
			}
		}
		//TODO: Handle PREGAP commands
	}
	DiskUtils::OpticalMediaPtr result;
	if(tracks.size() == 1)
	{
		//If we have one track, let's not worry too much and just use auto-detect.
		//Most likely a CD, but since we don't check the track mode, we could have
		//some other data type (ex.: cooked 2048 bytes per sector file) that we still
		//want to be able to read.
		result = COpticalMedia::CreateAuto(tracks[0].stream);
	}
	else if(tracks.size() > 1)
	{
		//We need to create some unified "file" using all the tracks files.
		//All the files need to have the same block size. We assume CD type blocks.
		std::vector<CMultiImageStream::StreamPtr> streams;
		for(auto& track : tracks)
		{
			uint64 trackSize = track.stream->GetLength();
			if((trackSize % COpticalMedia::MEDIA_BLOCK_SIZE_2352) != 0)
			{
				throw std::runtime_error("Inconsistent track block size.");
			}
			streams.push_back(track.stream);
			track.size = trackSize;
		}
		auto discStream = std::make_shared<CMultiImageStream>(streams);
		result = COpticalMedia::CreateAuto(discStream, COpticalMedia::CREATE_AUTO_NO_FIRST_TRACK);
		uint64 currentTrackPosition = 0;
		for(const auto& track : tracks)
		{
			COpticalMedia::TRACK newTrack = {};
			newTrack.start = static_cast<uint32>(currentTrackPosition / COpticalMedia::MEDIA_BLOCK_SIZE_2352);
			newTrack.size = static_cast<uint32>(track.size / COpticalMedia::MEDIA_BLOCK_SIZE_2352);
			result->AddTrack(newTrack);
			currentTrackPosition += track.size;
		}
	}
	if(!result)
	{
		throw std::runtime_error("Could not build media from cuesheet.");
	}
	return result;
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
	auto imageStream = std::make_shared<CChdCdImageStream>(CreateImageStream(imagePath));
	auto trackInfo = [&imageStream]() -> std::pair<COpticalMedia::BlockProviderPtr, COpticalMedia::MEDIA_BLOCK_TYPE> {
		static constexpr uint64 CHD_CD_UNITSIZE = 2448;
		static constexpr uint64 CD_MEDIA_UNIT_SIZE = COpticalMedia::MEDIA_BLOCK_SIZE_2352;
		switch(imageStream->GetDataType())
		{
		default:
			assert(false);
			[[fallthrough]];
		case CChdCdImageStream::DATA_TYPE_CD_MODE1:
			return std::make_pair(std::make_shared<ISO9660::CBlockProviderCustom<CHD_CD_UNITSIZE, CD_MEDIA_UNIT_SIZE, 0>>(imageStream), COpticalMedia::MEDIA_BLOCK_TYPE_2352);
		case CChdCdImageStream::DATA_TYPE_CD_MODE1_RAW:
			return std::make_pair(std::make_shared<ISO9660::CBlockProviderCustom<CHD_CD_UNITSIZE, CD_MEDIA_UNIT_SIZE, 0x10>>(imageStream), COpticalMedia::MEDIA_BLOCK_TYPE_2352);
		case CChdCdImageStream::DATA_TYPE_CD_MODE2_RAW:
			return std::make_pair(std::make_shared<ISO9660::CBlockProviderCustom<CHD_CD_UNITSIZE, CD_MEDIA_UNIT_SIZE, 0x18>>(imageStream), COpticalMedia::MEDIA_BLOCK_TYPE_2352);
		case CChdCdImageStream::DATA_TYPE_DVD:
			return std::make_pair(std::make_shared<ISO9660::CBlockProvider2048>(imageStream), COpticalMedia::MEDIA_BLOCK_TYPE_2048);
		}
	}();
	std::vector<COpticalMedia::TRACK> tracks;
	const auto& chdTracks = imageStream->GetTracks();
	if(!chdTracks.empty())
	{
		uint32 trackPos = 0;
		for(int i = 0; i < chdTracks.size(); i++)
		{
			const auto& chdTrack = chdTracks[i];
			COpticalMedia::TRACK track = {};
			track.start = trackPos;
			track.size = chdTrack.frames;
			trackPos += chdTrack.frames;
			//CHD tracks start on a 4 sector boundary
			trackPos = (trackPos + 0x03) & ~0x03;
			tracks.push_back(track);
		}
	}
	else
	{
		//Create first track
		COpticalMedia::TRACK track = {};
		track.size = trackInfo.first->GetBlockCount();
		tracks.push_back(track);
	}
	auto result = COpticalMedia::CreateCustom(std::move(trackInfo.first), trackInfo.second, std::move(tracks));
	return result;
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
