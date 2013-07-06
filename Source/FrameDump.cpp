#include "FrameDump.h"
#include "MemoryStateFile.h"
#include "RegisterStateFile.h"

#define STATE_INITIAL_GSRAM				"init/gsram"
#define STATE_INITIAL_GSREGS			"init/gsregs"
#define STATE_PACKET_PREFIX				"packet_"
#define STATE_PACKET_METADATA_PREFIX	"packetmetadata_"

CFrameDump::CFrameDump()
{
	m_initialGsRam = new uint8[CGSHandler::RAMSIZE];
	Reset();
}

CFrameDump::~CFrameDump()
{
	delete [] m_initialGsRam;
}

void CFrameDump::Reset()
{
	m_packets.clear();
}

uint8* CFrameDump::GetInitialGsRam()
{
	return m_initialGsRam;
}

uint64* CFrameDump::GetInitialGsRegisters()
{
	return m_initialGsRegisters;
}

const CFrameDump::PacketArray& CFrameDump::GetPackets() const
{
	return m_packets;
}

void CFrameDump::AddPacket(const CGSHandler::RegisterWrite* registerWrites, uint32 count, const CGsPacketMetadata* metadata)
{
	CGsPacket packet;
	packet.writes = CGsPacket::WriteArray(registerWrites, registerWrites + count);
	if(metadata)
	{
		packet.metadata = *metadata;
	}
	m_packets.push_back(packet);
}

void CFrameDump::Read(Framework::CStream& input)
{
	Reset();

	Framework::CZipArchiveReader archive(input);
	
	archive.BeginReadFile(STATE_INITIAL_GSRAM)->Read(m_initialGsRam, CGSHandler::RAMSIZE);
	archive.BeginReadFile(STATE_INITIAL_GSREGS)->Read(m_initialGsRegisters, sizeof(uint64) * CGSHandler::REGISTER_MAX);

	std::map<unsigned int, std::string> packetFiles;
	for(const auto& fileHeader : archive.GetFileHeaders())
	{
		if(fileHeader.first.find(STATE_PACKET_PREFIX) == 0)
		{
			unsigned int packetIdx = 0;
			sscanf(fileHeader.first.c_str(), STATE_PACKET_PREFIX "%d", &packetIdx);
			packetFiles[packetIdx] = fileHeader.first;
		}
	}

	for(const auto& packetFilePair : packetFiles)
	{
		const auto& packetFile = packetFilePair.second;
		const auto& packetFileHeader = archive.GetFileHeader(packetFile.c_str());
		unsigned int writeCount = packetFileHeader->uncompressedSize / sizeof(CGSHandler::RegisterWrite);
		assert(packetFileHeader->uncompressedSize % sizeof(CGSHandler::RegisterWrite) == 0);

		std::string packetMetadataFile = STATE_PACKET_METADATA_PREFIX + std::to_string(packetFilePair.first);

		CGsPacket packet;
		packet.writes.resize(writeCount);

		archive.BeginReadFile(packetFile.c_str())->Read(packet.writes.data(), packet.writes.size() * sizeof(CGSHandler::RegisterWrite));
		archive.BeginReadFile(packetMetadataFile.c_str())->Read(&packet.metadata, sizeof(CGsPacketMetadata));

		m_packets.push_back(packet);
	}
}

void CFrameDump::Write(Framework::CStream& output) const
{
	Framework::CZipArchiveWriter archive;

	archive.InsertFile(new CMemoryStateFile(STATE_INITIAL_GSRAM,	m_initialGsRam,			CGSHandler::RAMSIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_INITIAL_GSREGS,	m_initialGsRegisters,	sizeof(uint64) * CGSHandler::REGISTER_MAX));

	unsigned int currentPacket = 0;
	for(const auto& packet : m_packets)
	{
		std::string packetName = STATE_PACKET_PREFIX + std::to_string(currentPacket);
		std::string packetMetadataName = STATE_PACKET_METADATA_PREFIX + std::to_string(currentPacket);
		archive.InsertFile(new CMemoryStateFile(packetName.c_str(), packet.writes.data(), packet.writes.size() * sizeof(CGSHandler::RegisterWrite)));
		archive.InsertFile(new CMemoryStateFile(packetMetadataName.c_str(), &packet.metadata, sizeof(CGsPacketMetadata)));
		currentPacket++;
	}

	archive.Write(output);
}
