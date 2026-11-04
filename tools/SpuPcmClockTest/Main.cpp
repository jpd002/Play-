#include <cstdio>
#include <vector>
#include <stdexcept>
#include "AppConfig.h"
#include "PathUtils.h"
#include "Ps2Const.h"
#include "iop/Iop_SpuBase.h"

fs::path CAppConfig::GetBasePath() const
{
	auto path = fs::current_path() / "pcm-clock-test-data";
	Framework::PathUtils::EnsurePathExists(path);
	return path;
}

static void CheckRate(uint32 sourceRate, uint32 destinationRate)
{
	std::vector<uint8> ram(PS2::SPU_RAM_SIZE);
	Iop::CSpuSampleCache cache;
	Iop::CSpuIrqWatcher watcher;
	watcher.Reset();
	Iop::CSpuBase core(ram.data(), PS2::SPU_RAM_SIZE, &cache, &watcher, 0);
	core.SetDestinationSamplingRate(destinationRate);
	core.SetBaseSamplingRate(sourceRate);
	core.SetControl(0xC000);
	core.SetTransferMode(Iop::CSpuBase::TRANSFER_MODE_BLOCK_CORE0IN);
	core.m_inputVolL = core.m_inputVolR = 0x8000;
	std::vector<int16> block(512, 1024);
	int16 output[2] = {};
	unsigned int nextBlock = 0;
	constexpr unsigned int seconds = 4;
	for(unsigned int frame = 0; frame < destinationRate * seconds; frame++)
	{
		auto accepted = core.ReceiveDma(reinterpret_cast<uint8*>(block.data()), 16, 64, 0);
		if(accepted)
		{
			if(accepted != 64) throw std::runtime_error("Unexpected partial PCM block");
			++nextBlock;
			std::fill(block.begin(), block.end(), static_cast<int16>(1024 + nextBlock));
		}
		core.Render(output, 2);
	}
	//Each complete DMA block contains 256 stereo input frames. Its marker
	//identifies which point in the source stream is playing after four seconds.
	const unsigned int expected = ((sourceRate * seconds) - 1) / 256;
	const unsigned int actual = output[0] - 1024;
	printf("PCM %u -> %u: source block %u after %u seconds, expected %u\n",
	       sourceRate, destinationRate, actual, seconds, expected);
	if(actual != expected || output[0] != output[1]) throw std::runtime_error("PCM source clock drifts across DMA blocks");
}

int main()
{
	try
	{
		CheckRate(48000, 48000);
		CheckRate(48000, 44100);
		puts("PASS: native and resampled PCM clocks");
		return 0;
	}
	catch(const std::exception& error) { fprintf(stderr, "FAIL: %s\n", error.what()); return 1; }
}
