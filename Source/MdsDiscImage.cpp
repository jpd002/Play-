#include "MdsDiscImage.h"
#include "StdStreamUtils.h"

enum MDS_MEDIUM : uint16
{
	CD = 0,
	CD_R = 1,
	CD_RW = 2,
	DVD = 0x10,
	DVD_MINUS_R = 0x12,
};

static const char g_mdsSignature[17] = "MEDIA DESCRIPTOR";

struct MDS_HEADER
{
	uint8 signature[0x10];
	uint8 version[2];
	MDS_MEDIUM mediumType;
	uint16 sessionCount;
	uint16 dummy1[2];
	uint16 bcaLength;
	uint32 dummy2[2];
	uint32 bcaDataOffset;
	uint32 dummy3[6];
	uint32 discStructuresOffset;
	uint32 dummy4[3];
	uint32 sessionsBlocksOffset;
	uint32 dpmBlocksOffset;
};
static_assert(sizeof(MDS_HEADER) == 88, "MDS_HEADER must be 88 bytes long.");

//Read Disc Structure - Command 0x00
struct DVD_RDS_PHYSICALFORMATINFO
{
	uint8 partVersion : 4;
	uint8 bookType : 4;
	uint8 maximumRate : 4;
	uint8 discSize : 4;
	uint8 layerType : 4;
	uint8 trackPath : 1;
	uint8 numberOfLayers : 2;
	uint8 reserved0 : 1;
	uint8 trackDensity : 4;
	uint8 linearDensity : 4;
	uint8 startSectorNumber[4];
	uint8 endSectorNumber[4];
	uint8 endSectorNumberLayer0[4];
	uint8 reserved1 : 7;
	uint8 bcaFlag : 1;
	uint8 mediaSpecific[2031];

	uint32 GetStartSectorNumber() const
	{
		return (startSectorNumber[1] << 16) | (startSectorNumber[2] << 8) | (startSectorNumber[3]);
	}

	uint32 GetEndSectorNumber() const
	{
		return (endSectorNumber[1] << 16) | (endSectorNumber[2] << 8) | (endSectorNumber[3]);
	}
};
static_assert(sizeof(DVD_RDS_PHYSICALFORMATINFO) == 2048, "DVD_RDS_PHYSICALFORMATINFO must be 2048 bytes long.");

//Read Disc Structure - Command 0x01
struct DVD_RDS_COPYRIGHTINFO
{
	uint32 info;
};
static_assert(sizeof(DVD_RDS_COPYRIGHTINFO) == 4, "DVD_RDS_COPYRIGHTINFO must be 4 bytes long.");

//Read Disc Structure - Command 0x04
struct DVD_RDS_MANUFACTURINGINFO
{
	uint8 info[2048];
};
static_assert(sizeof(DVD_RDS_MANUFACTURINGINFO) == 2048, "DVD_RDS_MANUFACTURINGINFO must be 2048 bytes long.");

CMdsDiscImage::CMdsDiscImage(const boost::filesystem::path& inputPath)
{
	auto inputStream = Framework::CreateInputStdStream(inputPath.native());
	ParseMds(inputStream);
}

void CMdsDiscImage::ParseMds(Framework::CStream& inputStream)
{
	MDS_HEADER header = {};
	inputStream.Read(&header, sizeof(MDS_HEADER));
	if(memcmp(header.signature, g_mdsSignature, 16) != 0)
	{
		throw std::runtime_error("Invalid MDS file.");
	}
	if(header.version[0] != 0x01)
	{
		throw std::runtime_error("Invalid MDS file version.");
	}

	if(header.mediumType == MDS_MEDIUM::DVD)
	{
		inputStream.Seek(header.discStructuresOffset, Framework::STREAM_SEEK_SET);

		DVD_RDS_COPYRIGHTINFO copyrightInfo = {};
		inputStream.Read(&copyrightInfo, sizeof(DVD_RDS_COPYRIGHTINFO));

		DVD_RDS_MANUFACTURINGINFO manufacturingInfo = {};
		inputStream.Read(&manufacturingInfo, sizeof(DVD_RDS_MANUFACTURINGINFO));

		DVD_RDS_PHYSICALFORMATINFO physicalFormatInfo = {};
		inputStream.Read(&physicalFormatInfo, sizeof(DVD_RDS_PHYSICALFORMATINFO));

		uint32 startSectorNumber = physicalFormatInfo.GetStartSectorNumber();
		uint32 endSectorNumber = physicalFormatInfo.GetEndSectorNumber();

		if(physicalFormatInfo.numberOfLayers == 1)
		{
			m_isDualLayer = true;
			m_layerBreak = endSectorNumber - startSectorNumber + 1;
		}
	}
}
