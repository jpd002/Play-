#include "FrameDump.h"
#include "MemoryStateFile.h"
#include "RegisterStateFile.h"

#define STATE_INITIAL_GSRAM			"init/gsram"
#define STATE_INITIAL_GSREGS		"init/gsregs"
#define STATE_PACKET_PREFIX			"packet_"

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

void CFrameDump::AddPacket(const CGSHandler::RegisterWrite* registerWrites, uint32 count)
{
	m_packets.push_back(Packet(registerWrites, registerWrites + count));
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

		Packet packet;
		packet.resize(writeCount);

		archive.BeginReadFile(packetFile.c_str())->Read(packet.data(), packet.size() * sizeof(CGSHandler::RegisterWrite));

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
		std::string packetName = STATE_PACKET_PREFIX + std::to_string(currentPacket++);
		archive.InsertFile(new CMemoryStateFile(packetName.c_str(), packet.data(), packet.size() * sizeof(CGSHandler::RegisterWrite)));
	}

	archive.Write(output);
}
