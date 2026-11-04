#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include "OpticalMedia.h"
#include "AppConfig.h"
#include "PathUtils.h"
#include "Ps2Const.h"
#include "iop/Iop_SubSystem.h"
#include "iop/IopBios.h"
#include "iop/Iop_Cdvdman.h"
#include "iop/Iop_SifManNull.h"
#include "iop/namco_sys246/Iop_NamcoAcCdvd.h"
#include "iop/namco_sys246/Iop_NamcoAcRam.h"

//No copyrighted disc or BIOS data is needed: every payload sector records its LBA.
class TestDisc : public ISO9660::CBlockProvider
{
public:
	std::vector<uint32> reads;
	void ReadBlock(uint32 lba, void* output) override
	{
		auto bytes = static_cast<uint8*>(output);
		memset(bytes, 0, BLOCKSIZE);
		if(lba == 16)
		{
			bytes[0] = 1;
			memcpy(bytes + 1, "CD001", 5);
			bytes[6] = 1;
			bytes[140] = 17; //Empty path table, sufficient for raw streaming reads.
		}
		else if(lba != 17)
		{
			reads.push_back(lba);
			for(size_t offset = 0; offset < BLOCKSIZE; offset += sizeof(lba))
			{
				memcpy(bytes + offset, &lba, sizeof(lba));
			}
		}
	}
	void ReadMediaBlock(uint32 lba, void* output) override { ReadBlock(lba, output); }
	uint32 GetBlockCount() override { return 4096; }
	uint32 GetMediaBlockSize() const override { return BLOCKSIZE; }
};

static void Check(bool condition, const char* message)
{
	if(!condition) throw std::runtime_error(message);
}

fs::path CAppConfig::GetBasePath() const
{
	auto path = fs::current_path() / "NamcoAcCdvdTest-data";
	Framework::PathUtils::EnsurePathExists(path);
	return path;
}

int main()
{
	try
	{
		Iop::CSubSystem subSystem(true);
		subSystem.Reset();
		auto bios = static_cast<CIopBios*>(subSystem.m_bios.get());
		auto sif = std::make_shared<Iop::CSifManNull>();
		bios->Reset(PS2::IOP_BASE_RAM_SIZE, sif);
		auto acRam = std::make_unique<Iop::Namco::CAcRam>(*bios, subSystem.m_ram);
		Iop::Namco::CAcCdvd cdvd(*sif, *bios->GetCdvdman(), subSystem.m_ram, *acRam);
		auto disc = std::make_shared<TestDisc>();
		auto media = COpticalMedia::CreateCustom(disc, COpticalMedia::MEDIA_BLOCK_TYPE_2048, {});
		cdvd.SetOpticalMedia(media.get());
		disc->reads.clear();
		std::vector<uint8> ram(0x10000, 0xA5);
		uint32 reply[16] = {};
		auto seek = [&](uint32 lba) {
			uint32 args[] = {lba};
			cdvd.Invoke(0x15, args, sizeof(args), reply, sizeof(reply), ram.data());
		};
		auto read = [&](uint32 first, uint32 count, bool errorPointer) {
			constexpr uint32 dst = 0x1000;
			constexpr uint32 err = 0x800;
			std::fill(ram.begin(), ram.end(), 0xA5);
			disc->reads.clear();
			uint32 args[] = {count, 0, dst, errorPointer ? err : 0};
			cdvd.Invoke(0x13, args, sizeof(args), reply, sizeof(reply), ram.data());
			Check(disc->reads.size() == count, "wrong number of sector reads");
			for(uint32 i = 0; i < count; i++)
			{
				if(disc->reads[i] != first + i)
				{
					printf("sector %u: expected LBA %u, got %u\n", i, first + i, disc->reads[i]);
					throw std::runtime_error("stream skipped or repeated a disc sector");
				}
				for(uint32 offset = 0; offset < 0x800; offset += sizeof(uint32))
				{
					uint32 value = 0;
					memcpy(&value, ram.data() + dst + i * 0x800 + offset, sizeof(value));
					Check(value == first + i, "stream payload or destination offset is incorrect");
				}
			}
			Check(ram[dst - 1] == 0xA5 && ram[dst + count * 0x800] == 0xA5, "destination overrun");
			uint32 error = 0;
			memcpy(&error, ram.data() + err, sizeof(error));
			Check(error == (errorPointer ? 0 : 0xA5A5A5A5), "incorrect error-pointer handling");
			Check(ram[0] == 0xA5, "null error pointer wrote to address zero");
		};
		seek(100);
		read(100, 4, true);
		read(104, 3, false);
		read(107, 1, true);
		read(108, 0, false);
		read(108, 2, true);
		seek(200);
		read(200, 3, false);
		puts("PASS: multi-sector reads, successive reads, zero/one-sector reads, seek, payloads and bounds");
		return 0;
	}
	catch(const std::exception& exception)
	{
		fprintf(stderr, "FAIL: %s\n", exception.what());
		return 1;
	}
}
