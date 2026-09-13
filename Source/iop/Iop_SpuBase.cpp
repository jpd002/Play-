#include <cassert>
#include <cstring>
#include <cmath>
#include <climits>
#include <algorithm>
#include "string_format.h"
#include "Log.h"
#include "../states/RegisterStateCollectionFile.h"
#include "../states/RegisterStateUtils.h"
#include "../states/RegisterStateFile.h"
#include "Iop_SpuBase.h"

using namespace Iop;

#define INIT_SAMPLE_RATE (44100)
#define PITCH_BASE (0x1000)
#define TIME_SCALE (0x1000)
#define RESET_IRQ_ADDR (~0U)
#define LOG_NAME ("iop_spubase")

#define STATE_REGS_PATH_FORMAT ("iop_spu/spu_%d.xml")

#define STATE_REGS ("GlobalRegs")
#define STATE_REGS_CTRL ("CTRL")
#define STATE_REGS_IRQADDR ("IRQADDR")
#define STATE_REGS_IRQPENDING ("IRQPENDING")
#define STATE_REGS_TRANSFERADDR ("TRANSFERADDR")
#define STATE_REGS_TRANSFERMODE ("TRANSFERMODE")
#define STATE_REGS_CORE0OUTPUTOFFSET ("CORE0OUTPUTOFFSET")
#define STATE_REGS_CHANNELON ("CHANNELON")
#define STATE_REGS_CHANNELREVERB ("CHANNELREVERB")
#define STATE_REGS_REVERBWORKADDRSTART ("REVERBWORKADDRSTART")
#define STATE_REGS_REVERBWORKADDREND ("REVERBWORKADDREND")
#define STATE_REGS_REVERBCURRADDR ("REVERBCURRADDR")
#define STATE_REGS_REVERB_FORMAT ("REVERB%d")
#define STATE_REGS_INPVOLUMELEFT ("INPVOLUMELEFT")
#define STATE_REGS_INPVOLUMERIGHT ("INPVOLUMERIGHT")
#define STATE_REGS_EXTINPVOLUMELEFT ("EXTINPVOLUMELEFT")
#define STATE_REGS_EXTINPVOLUMERIGHT ("EXTINPVOLUMERIGHT")

#define STATE_CHANNEL_REGS_FORMAT ("Channel%02dRegs")
#define STATE_CHANNEL_REGS_VOLUMELEFT ("VOLUMELEFT")
#define STATE_CHANNEL_REGS_VOLUMERIGHT ("VOLUMERIGHT")
#define STATE_CHANNEL_REGS_VOLUMELEFTABS ("VOLUMELEFTABS")
#define STATE_CHANNEL_REGS_VOLUMERIGHTABS ("VOLUMERIGHTABS")
#define STATE_CHANNEL_REGS_STATUS ("STATUS")
#define STATE_CHANNEL_REGS_PITCH ("PITCH")
#define STATE_CHANNEL_REGS_ADSRLEVEL ("ADSRLEVEL")
#define STATE_CHANNEL_REGS_ADSRRATE ("ADSRRATE")
#define STATE_CHANNEL_REGS_ADSRVOLUME ("ADSRVOLUME")
#define STATE_CHANNEL_REGS_ADDRESS ("ADDRESS")
#define STATE_CHANNEL_REGS_REPEAT ("REPEAT")
#define STATE_CHANNEL_REGS_REPEATSET ("REPEATSET")
#define STATE_CHANNEL_REGS_CURRENT ("CURRENT")

#define STATE_SAMPLEREADER_REGS_SRCSAMPLEIDX ("SR_SrcSampleIdx")
#define STATE_SAMPLEREADER_REGS_SRCSAMPLINGRATE ("SR_SrcSamplingRate")
#define STATE_SAMPLEREADER_REGS_NEXTSAMPLEADDR ("SR_NextSampleAddr")
#define STATE_SAMPLEREADER_REGS_REPEATADDR ("SR_RepeatAddr")
#define STATE_SAMPLEREADER_REGS_PITCH ("SR_Pitch")
#define STATE_SAMPLEREADER_REGS_S1 ("SR_S1")
#define STATE_SAMPLEREADER_REGS_S2 ("SR_S2")
#define STATE_SAMPLEREADER_REGS_DONE ("SR_Done")
#define STATE_SAMPLEREADER_REGS_NEXTVALID ("SR_NextValid")
#define STATE_SAMPLEREADER_REGS_ENDFLAG ("SR_EndFlag")
#define STATE_SAMPLEREADER_REGS_DIDCHANGEREPEAT ("SR_DidChangeRepeat")
#define STATE_SAMPLEREADER_REGS_BUFFER_FORMAT ("SR_Buffer%d")

#define STATE_IRQWATCHER_REGS_PATH ("iop_spu/spu_irqwatcher.xml")

#define STATE_IRQWATCHER_REGS_IRQADDR0 ("irqAddr0")
#define STATE_IRQWATCHER_REGS_IRQADDR1 ("irqAddr1")
#define STATE_IRQWATCHER_REGS_IRQPENDING0 ("irqPending0")
#define STATE_IRQWATCHER_REGS_IRQPENDING1 ("irqPending1")

// clang-format off
bool CSpuBase::g_reverbParamIsAddress[REVERB_PARAM_COUNT] =
{
	true,
	true,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	false,
	false
};

const uint32 CSpuBase::g_linearIncreaseSweepDeltas[0x80] =
{
	0x3A0CC55E, 0x305FF9CE, 0x2976D61E, 0x203FFBDE, 0x1D0662AF, 0x182FFCE7, 0x1359971F, 0x101FFDEF,
	0x0DD2475F, 0x0C17FE73, 0x0A0233AF, 0x080FFEF7, 0x07144A05, 0x060BFF39, 0x050119D7, 0x03F9DC6C,
	0x037F3A27, 0x02FE04E5, 0x026B32E3, 0x01EF5BE9, 0x01B514DD, 0x018712AA, 0x01430F6B, 0x0100385E,
	0x00E129C7, 0x00BE85CF, 0x00A187B5, 0x00801C2F, 0x007094E3, 0x00607F9E, 0x004FE588, 0x003DEB7D,
	0x00392824, 0x00318930, 0x00271B77, 0x00204E57, 0x001B851B, 0x0017F80F, 0x00141506, 0x0010272B,
	0x000E0504, 0x000BFC07, 0x000A0A83, 0x0007FD5A, 0x0006C140, 0x00063126, 0x0004F41E, 0x0003E925,
	0x000389CC, 0x0002F8DF, 0x00027A0F, 0x0002021A, 0x0001C4E6, 0x00017C6F, 0x00014267, 0x0001010D,
	0x0000DFC9, 0x0000C023, 0x00009E83, 0x00007ECF, 0x00006FE4, 0x00005F1B, 0x00004F41, 0x00003F67,
	0x000037F2, 0x00002F8D, 0x000027A0, 0x0000203D, 0x00001BF9, 0x00001814, 0x00001405, 0x00000FD9,
	0x00000D96, 0x00000BE3, 0x00000A02, 0x000007EC, 0x0000070B, 0x000005F1, 0x00000501, 0x000003F6,
	0x00000385, 0x00000304, 0x00000280, 0x00000200, 0x000001BE, 0x0000017F, 0x00000140, 0x00000100,
	0x000000DF, 0x000000BF, 0x000000A0, 0x00000080, 0x0000006F, 0x0000005F, 0x00000050, 0x00000040,
	0x00000037, 0x0000002F, 0x00000028, 0x00000020, 0x0000001B, 0x00000017, 0x00000014, 0x00000010,
	0x0000000D, 0x0000000B, 0x0000000A, 0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

const uint32 CSpuBase::g_linearDecreaseSweepDeltas[0x80] =
{
	0x488FF6B5, 0x3A0CC55E, 0x305FF9CE, 0x2976D61E, 0x203FFBDE, 0x1D0662AF, 0x182FFCE7, 0x1359971F,
	0x101FFDEF, 0x0DD2475F, 0x0C17FE73, 0x0A0233AF, 0x080FFEF7, 0x07144A05, 0x060BFF39, 0x050119D7,
	0x03F9DC6C, 0x037F3A27, 0x02FE04E5, 0x026B32E3, 0x01EF5BE9, 0x01B514DD, 0x018712AA, 0x01430F6B,
	0x0100385E, 0x00E129C7, 0x00BE85CF, 0x00A187B5, 0x00801C2F, 0x007094E3, 0x00607F9E, 0x004FE588,
	0x003DEB7D, 0x00392824, 0x00318930, 0x00271B77, 0x00204E57, 0x001B851B, 0x0017F80F, 0x00141506,
	0x0010272B, 0x000E0504, 0x000BFC07, 0x000A0A83, 0x0007FD5A, 0x0006C140, 0x00063126, 0x0004F41E,
	0x0003E925, 0x000389CC, 0x0002F8DF, 0x00027A0F, 0x0002021A, 0x0001C4E6, 0x00017C6F, 0x00014267,
	0x0001010D, 0x0000DFC9, 0x0000C023, 0x00009E83, 0x00007ECF, 0x00006FE4, 0x00005F1B, 0x00004F41,
	0x00003F67, 0x000037F2, 0x00002F8D, 0x000027A0, 0x0000203D, 0x00001BF9, 0x00001814, 0x00001405,
	0x00000FD9, 0x00000D96, 0x00000BE3, 0x00000A02, 0x000007EC, 0x0000070B, 0x000005F1, 0x00000501,
	0x000003F6, 0x00000385, 0x00000304, 0x00000280, 0x00000200, 0x000001BE, 0x0000017F, 0x00000140,
	0x00000100, 0x000000DF, 0x000000BF, 0x000000A0, 0x00000080, 0x0000006F, 0x0000005F, 0x00000050,
	0x00000040, 0x00000037, 0x0000002F, 0x00000028, 0x00000020, 0x0000001B, 0x00000017, 0x00000014,
	0x00000010, 0x0000000D, 0x0000000B, 0x0000000A, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

const uint16 CSpuBase::CSampleReader::g_gaussShuffledReverseTable[1024] =
{
	0x12c7, 0x59b3, 0x1307, 0xffff, 0x1288, 0x59b2, 0x1347, 0xffff, 0x1249, 0x59b0, 0x1388, 0xffff, 0x120b, 0x59ad, 0x13c9, 0xffff,
	0x11cd, 0x59a9, 0x140b, 0xffff, 0x118f, 0x59a4, 0x144d, 0xffff, 0x1153, 0x599e, 0x1490, 0xffff, 0x1116, 0x5997, 0x14d4, 0xffff,
	0x10db, 0x598f, 0x1517, 0xffff, 0x109f, 0x5986, 0x155c, 0xffff, 0x1065, 0x597c, 0x15a0, 0xffff, 0x102a, 0x5971, 0x15e6, 0xffff,
	0x0ff1, 0x5965, 0x162c, 0xffff, 0x0fb7, 0x5958, 0x1672, 0xffff, 0x0f7f, 0x5949, 0x16b9, 0xffff, 0x0f46, 0x593a, 0x1700, 0xffff,
	0x0f0f, 0x592a, 0x1747, 0x0000, 0x0ed7, 0x5919, 0x1790, 0x0000, 0x0ea1, 0x5907, 0x17d8, 0x0000, 0x0e6b, 0x58f4, 0x1821, 0x0000,
	0x0e35, 0x58e0, 0x186b, 0x0000, 0x0e00, 0x58cb, 0x18b5, 0x0000, 0x0dcb, 0x58b5, 0x1900, 0x0000, 0x0d97, 0x589e, 0x194b, 0x0001,
	0x0d63, 0x5886, 0x1996, 0x0001, 0x0d30, 0x586d, 0x19e2, 0x0001, 0x0cfd, 0x5853, 0x1a2e, 0x0001, 0x0ccb, 0x5838, 0x1a7b, 0x0002,
	0x0c99, 0x581c, 0x1ac8, 0x0002, 0x0c68, 0x57ff, 0x1b16, 0x0002, 0x0c38, 0x57e2, 0x1b64, 0x0003, 0x0c07, 0x57c3, 0x1bb3, 0x0003,
	0x0bd8, 0x57a3, 0x1c02, 0x0003, 0x0ba9, 0x5782, 0x1c51, 0x0004, 0x0b7a, 0x5761, 0x1ca1, 0x0004, 0x0b4c, 0x573e, 0x1cf1, 0x0005,
	0x0b1e, 0x571b, 0x1d42, 0x0005, 0x0af1, 0x56f6, 0x1d93, 0x0006, 0x0ac4, 0x56d1, 0x1de5, 0x0007, 0x0a98, 0x56ab, 0x1e37, 0x0007,
	0x0a6c, 0x5684, 0x1e89, 0x0008, 0x0a40, 0x565b, 0x1edc, 0x0009, 0x0a16, 0x5632, 0x1f2f, 0x0009, 0x09eb, 0x5609, 0x1f82, 0x000a,
	0x09c1, 0x55de, 0x1fd6, 0x000b, 0x0998, 0x55b2, 0x202a, 0x000c, 0x096f, 0x5585, 0x207f, 0x000d, 0x0946, 0x5558, 0x20d4, 0x000e,
	0x091e, 0x5529, 0x2129, 0x000f, 0x08f7, 0x54fa, 0x217f, 0x0010, 0x08d0, 0x54ca, 0x21d5, 0x0011, 0x08a9, 0x5499, 0x222c, 0x0012,
	0x0883, 0x5467, 0x2282, 0x0013, 0x085d, 0x5434, 0x22da, 0x0015, 0x0838, 0x5401, 0x2331, 0x0016, 0x0813, 0x53cc, 0x2389, 0x0018,
	0x07ef, 0x5397, 0x23e1, 0x0019, 0x07cb, 0x5361, 0x2439, 0x001b, 0x07a7, 0x532a, 0x2492, 0x001c, 0x0784, 0x52f3, 0x24eb, 0x001e,
	0x0762, 0x52ba, 0x2545, 0x0020, 0x0740, 0x5281, 0x259e, 0x0021, 0x071e, 0x5247, 0x25f8, 0x0023, 0x06fd, 0x520c, 0x2653, 0x0025,
	0x06dc, 0x51d0, 0x26ad, 0x0027, 0x06bb, 0x5194, 0x2708, 0x0029, 0x069b, 0x5156, 0x2763, 0x002c, 0x067c, 0x5118, 0x27be, 0x002e,
	0x065c, 0x50da, 0x281a, 0x0030, 0x063e, 0x509a, 0x2876, 0x0033, 0x061f, 0x505a, 0x28d2, 0x0035, 0x0601, 0x5019, 0x292e, 0x0038,
	0x05e4, 0x4fd7, 0x298b, 0x003a, 0x05c7, 0x4f95, 0x29e7, 0x003d, 0x05aa, 0x4f52, 0x2a44, 0x0040, 0x058e, 0x4f0e, 0x2aa1, 0x0043,
	0x0572, 0x4ec9, 0x2aff, 0x0046, 0x0556, 0x4e84, 0x2b5c, 0x0049, 0x053b, 0x4e3e, 0x2bba, 0x004d, 0x0520, 0x4df7, 0x2c18, 0x0050,
	0x0506, 0x4db0, 0x2c76, 0x0054, 0x04ec, 0x4d68, 0x2cd4, 0x0057, 0x04d2, 0x4d20, 0x2d33, 0x005b, 0x04b9, 0x4cd7, 0x2d91, 0x005f,
	0x04a0, 0x4c8d, 0x2df0, 0x0063, 0x0488, 0x4c42, 0x2e4f, 0x0067, 0x0470, 0x4bf7, 0x2eae, 0x006b, 0x0458, 0x4bac, 0x2f0d, 0x006f,
	0x0441, 0x4b5f, 0x2f6c, 0x0074, 0x042a, 0x4b13, 0x2fcc, 0x0078, 0x0413, 0x4ac5, 0x302b, 0x007d, 0x03fc, 0x4a77, 0x308b, 0x0082,
	0x03e7, 0x4a29, 0x30ea, 0x0087, 0x03d1, 0x49d9, 0x314a, 0x008c, 0x03bc, 0x498a, 0x31aa, 0x0091, 0x03a7, 0x493a, 0x3209, 0x0096,
	0x0392, 0x48e9, 0x3269, 0x009c, 0x037e, 0x4898, 0x32c9, 0x00a1, 0x036a, 0x4846, 0x3329, 0x00a7, 0x0356, 0x47f4, 0x3389, 0x00ad,
	0x0343, 0x47a1, 0x33e9, 0x00b3, 0x0330, 0x474e, 0x3449, 0x00ba, 0x031d, 0x46fa, 0x34a9, 0x00c0, 0x030b, 0x46a6, 0x3509, 0x00c7,
	0x02f9, 0x4651, 0x3569, 0x00cd, 0x02e7, 0x45fc, 0x35c9, 0x00d4, 0x02d6, 0x45a6, 0x3629, 0x00db, 0x02c4, 0x4550, 0x3689, 0x00e3,
	0x02b4, 0x44fa, 0x36e8, 0x00ea, 0x02a3, 0x44a3, 0x3748, 0x00f2, 0x0293, 0x444c, 0x37a8, 0x00fa, 0x0283, 0x43f4, 0x3807, 0x0101,
	0x0273, 0x439c, 0x3867, 0x010a, 0x0264, 0x4344, 0x38c6, 0x0112, 0x0255, 0x42eb, 0x3926, 0x011b, 0x0246, 0x4292, 0x3985, 0x0123,
	0x0237, 0x4239, 0x39e4, 0x012c, 0x0229, 0x41df, 0x3a43, 0x0135, 0x021b, 0x4185, 0x3aa2, 0x013f, 0x020d, 0x412a, 0x3b00, 0x0148,
	0x0200, 0x40d0, 0x3b5f, 0x0152, 0x01f2, 0x4074, 0x3bbd, 0x015c, 0x01e5, 0x4019, 0x3c1b, 0x0166, 0x01d9, 0x3fbd, 0x3c79, 0x0171,
	0x01cc, 0x3f62, 0x3cd7, 0x017b, 0x01c0, 0x3f05, 0x3d35, 0x0186, 0x01b4, 0x3ea9, 0x3d92, 0x0191, 0x01a8, 0x3e4c, 0x3def, 0x019c,
	0x019c, 0x3def, 0x3e4c, 0x01a8, 0x0191, 0x3d92, 0x3ea9, 0x01b4, 0x0186, 0x3d35, 0x3f05, 0x01c0, 0x017b, 0x3cd7, 0x3f62, 0x01cc,
	0x0171, 0x3c79, 0x3fbd, 0x01d9, 0x0166, 0x3c1b, 0x4019, 0x01e5, 0x015c, 0x3bbd, 0x4074, 0x01f2, 0x0152, 0x3b5f, 0x40d0, 0x0200,
	0x0148, 0x3b00, 0x412a, 0x020d, 0x013f, 0x3aa2, 0x4185, 0x021b, 0x0135, 0x3a43, 0x41df, 0x0229, 0x012c, 0x39e4, 0x4239, 0x0237,
	0x0123, 0x3985, 0x4292, 0x0246, 0x011b, 0x3926, 0x42eb, 0x0255, 0x0112, 0x38c6, 0x4344, 0x0264, 0x010a, 0x3867, 0x439c, 0x0273,
	0x0101, 0x3807, 0x43f4, 0x0283, 0x00fa, 0x37a8, 0x444c, 0x0293, 0x00f2, 0x3748, 0x44a3, 0x02a3, 0x00ea, 0x36e8, 0x44fa, 0x02b4,
	0x00e3, 0x3689, 0x4550, 0x02c4, 0x00db, 0x3629, 0x45a6, 0x02d6, 0x00d4, 0x35c9, 0x45fc, 0x02e7, 0x00cd, 0x3569, 0x4651, 0x02f9,
	0x00c7, 0x3509, 0x46a6, 0x030b, 0x00c0, 0x34a9, 0x46fa, 0x031d, 0x00ba, 0x3449, 0x474e, 0x0330, 0x00b3, 0x33e9, 0x47a1, 0x0343,
	0x00ad, 0x3389, 0x47f4, 0x0356, 0x00a7, 0x3329, 0x4846, 0x036a, 0x00a1, 0x32c9, 0x4898, 0x037e, 0x009c, 0x3269, 0x48e9, 0x0392,
	0x0096, 0x3209, 0x493a, 0x03a7, 0x0091, 0x31aa, 0x498a, 0x03bc, 0x008c, 0x314a, 0x49d9, 0x03d1, 0x0087, 0x30ea, 0x4a29, 0x03e7,
	0x0082, 0x308b, 0x4a77, 0x03fc, 0x007d, 0x302b, 0x4ac5, 0x0413, 0x0078, 0x2fcc, 0x4b13, 0x042a, 0x0074, 0x2f6c, 0x4b5f, 0x0441,
	0x006f, 0x2f0d, 0x4bac, 0x0458, 0x006b, 0x2eae, 0x4bf7, 0x0470, 0x0067, 0x2e4f, 0x4c42, 0x0488, 0x0063, 0x2df0, 0x4c8d, 0x04a0,
	0x005f, 0x2d91, 0x4cd7, 0x04b9, 0x005b, 0x2d33, 0x4d20, 0x04d2, 0x0057, 0x2cd4, 0x4d68, 0x04ec, 0x0054, 0x2c76, 0x4db0, 0x0506,
	0x0050, 0x2c18, 0x4df7, 0x0520, 0x004d, 0x2bba, 0x4e3e, 0x053b, 0x0049, 0x2b5c, 0x4e84, 0x0556, 0x0046, 0x2aff, 0x4ec9, 0x0572,
	0x0043, 0x2aa1, 0x4f0e, 0x058e, 0x0040, 0x2a44, 0x4f52, 0x05aa, 0x003d, 0x29e7, 0x4f95, 0x05c7, 0x003a, 0x298b, 0x4fd7, 0x05e4,
	0x0038, 0x292e, 0x5019, 0x0601, 0x0035, 0x28d2, 0x505a, 0x061f, 0x0033, 0x2876, 0x509a, 0x063e, 0x0030, 0x281a, 0x50da, 0x065c,
	0x002e, 0x27be, 0x5118, 0x067c, 0x002c, 0x2763, 0x5156, 0x069b, 0x0029, 0x2708, 0x5194, 0x06bb, 0x0027, 0x26ad, 0x51d0, 0x06dc,
	0x0025, 0x2653, 0x520c, 0x06fd, 0x0023, 0x25f8, 0x5247, 0x071e, 0x0021, 0x259e, 0x5281, 0x0740, 0x0020, 0x2545, 0x52ba, 0x0762,
	0x001e, 0x24eb, 0x52f3, 0x0784, 0x001c, 0x2492, 0x532a, 0x07a7, 0x001b, 0x2439, 0x5361, 0x07cb, 0x0019, 0x23e1, 0x5397, 0x07ef,
	0x0018, 0x2389, 0x53cc, 0x0813, 0x0016, 0x2331, 0x5401, 0x0838, 0x0015, 0x22da, 0x5434, 0x085d, 0x0013, 0x2282, 0x5467, 0x0883,
	0x0012, 0x222c, 0x5499, 0x08a9, 0x0011, 0x21d5, 0x54ca, 0x08d0, 0x0010, 0x217f, 0x54fa, 0x08f7, 0x000f, 0x2129, 0x5529, 0x091e,
	0x000e, 0x20d4, 0x5558, 0x0946, 0x000d, 0x207f, 0x5585, 0x096f, 0x000c, 0x202a, 0x55b2, 0x0998, 0x000b, 0x1fd6, 0x55de, 0x09c1,
	0x000a, 0x1f82, 0x5609, 0x09eb, 0x0009, 0x1f2f, 0x5632, 0x0a16, 0x0009, 0x1edc, 0x565b, 0x0a40, 0x0008, 0x1e89, 0x5684, 0x0a6c,
	0x0007, 0x1e37, 0x56ab, 0x0a98, 0x0007, 0x1de5, 0x56d1, 0x0ac4, 0x0006, 0x1d93, 0x56f6, 0x0af1, 0x0005, 0x1d42, 0x571b, 0x0b1e,
	0x0005, 0x1cf1, 0x573e, 0x0b4c, 0x0004, 0x1ca1, 0x5761, 0x0b7a, 0x0004, 0x1c51, 0x5782, 0x0ba9, 0x0003, 0x1c02, 0x57a3, 0x0bd8,
	0x0003, 0x1bb3, 0x57c3, 0x0c07, 0x0003, 0x1b64, 0x57e2, 0x0c38, 0x0002, 0x1b16, 0x57ff, 0x0c68, 0x0002, 0x1ac8, 0x581c, 0x0c99,
	0x0002, 0x1a7b, 0x5838, 0x0ccb, 0x0001, 0x1a2e, 0x5853, 0x0cfd, 0x0001, 0x19e2, 0x586d, 0x0d30, 0x0001, 0x1996, 0x5886, 0x0d63,
	0x0001, 0x194b, 0x589e, 0x0d97, 0x0000, 0x1900, 0x58b5, 0x0dcb, 0x0000, 0x18b5, 0x58cb, 0x0e00, 0x0000, 0x186b, 0x58e0, 0x0e35,
	0x0000, 0x1821, 0x58f4, 0x0e6b, 0x0000, 0x17d8, 0x5907, 0x0ea1, 0x0000, 0x1790, 0x5919, 0x0ed7, 0x0000, 0x1747, 0x592a, 0x0f0f,
	0xffff, 0x1700, 0x593a, 0x0f46, 0xffff, 0x16b9, 0x5949, 0x0f7f, 0xffff, 0x1672, 0x5958, 0x0fb7, 0xffff, 0x162c, 0x5965, 0x0ff1,
	0xffff, 0x15e6, 0x5971, 0x102a, 0xffff, 0x15a0, 0x597c, 0x1065, 0xffff, 0x155c, 0x5986, 0x109f, 0xffff, 0x1517, 0x598f, 0x10db,
	0xffff, 0x14d4, 0x5997, 0x1116, 0xffff, 0x1490, 0x599e, 0x1153, 0xffff, 0x144d, 0x59a4, 0x118f, 0xffff, 0x140b, 0x59a9, 0x11cd,
	0xffff, 0x13c9, 0x59ad, 0x120b, 0xffff, 0x1388, 0x59b0, 0x1249, 0xffff, 0x1347, 0x59b2, 0x1288, 0xffff, 0x1307, 0x59b3, 0x12c7,
};

// clang-format on

CSpuBase::CSpuBase(uint8* ram, uint32 ramSize, CSpuSampleCache* sampleCache, CSpuIrqWatcher* irqWatcher, unsigned int spuNumber)
    : m_ram(ram)
    , m_ramSize(ramSize)
    , m_spuNumber(spuNumber)
    , m_sampleCache(sampleCache)
    , m_irqWatcher(irqWatcher)
    , m_reverbEnabled(true)
{
	Reset();

	//Init log table for ADSR
	memset(m_adsrLogTable, 0, sizeof(m_adsrLogTable));

	uint32 value = 3;
	uint32 columnIncrement = 1;
	uint32 column = 0;

	for(unsigned int i = 32; i < 160; i++)
	{
		if(value < 0x3FFFFFFF)
		{
			value += columnIncrement;
			column++;
			if(column == 5)
			{
				column = 1;
				columnIncrement *= 2;
			}
		}
		else
		{
			value = 0x3FFFFFFF;
		}
		m_adsrLogTable[i] = value;
	}
}

void CSpuBase::Reset()
{
	m_ctrl = 0;

	m_volumeAdjust = 1.0f;

	m_channelOn.f = 0;
	m_channelReverb.f = 0;
	m_reverbTicks = 0;
	m_irqAddr = RESET_IRQ_ADDR;
	m_irqPending = false;
	m_transferMode = 0;
	m_transferAddr = 0;

	m_core0OutputOffset = 0;

	m_reverbCurrAddr = 0;
	m_reverbWorkAddrStart = 0;
	m_reverbWorkAddrEnd = 0x80000;
	m_baseSamplingRate = INIT_SAMPLE_RATE;

	m_inputVolL = 0x7FFF;
	m_inputVolR = 0x7FFF;
	m_extInputVolL = 0x7FFF;
	m_extInputVolR = 0x7FFF;

	memset(m_channel, 0, sizeof(m_channel));
	memset(m_reverb, 0, sizeof(m_reverb));

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		auto& reader = m_reader[i];
		reader.Reset();
		reader.SetMemory(m_ram, m_ramSize);
		reader.SetSampleCache(m_sampleCache);
		reader.SetIrqWatcher(m_irqWatcher);
	}

	m_blockReader.Reset();
	m_soundInputDataAddr = (m_spuNumber == 0) ? SOUND_INPUT_DATA_CORE0_BASE : SOUND_INPUT_DATA_CORE1_BASE;
	m_blockWritePtr = 0;
}

void CSpuBase::LoadState(Framework::CZipArchiveReader& archive)
{
	auto path = string_format(STATE_REGS_PATH_FORMAT, m_spuNumber);
	auto stateCollectionFile = CRegisterStateCollectionFile(*archive.BeginReadFile(path.c_str()));

	{
		const auto& state = stateCollectionFile.GetRegisterState(STATE_REGS);
		m_ctrl = state.GetRegister32(STATE_REGS_CTRL);
		m_irqAddr = state.GetRegister32(STATE_REGS_IRQADDR);
		m_irqPending = state.GetRegister32(STATE_REGS_IRQPENDING) != 0;
		m_transferMode = state.GetRegister32(STATE_REGS_TRANSFERMODE);
		m_transferAddr = state.GetRegister32(STATE_REGS_TRANSFERADDR);
		m_core0OutputOffset = state.GetRegister32(STATE_REGS_CORE0OUTPUTOFFSET);
		m_channelOn.f = state.GetRegister32(STATE_REGS_CHANNELON);
		m_channelReverb.f = state.GetRegister32(STATE_REGS_CHANNELREVERB);
		m_reverbWorkAddrStart = state.GetRegister32(STATE_REGS_REVERBWORKADDRSTART);
		m_reverbWorkAddrEnd = state.GetRegister32(STATE_REGS_REVERBWORKADDREND);
		m_reverbCurrAddr = state.GetRegister32(STATE_REGS_REVERBCURRADDR);
		m_inputVolL = state.GetRegister32(STATE_REGS_INPVOLUMELEFT);
		m_inputVolR = state.GetRegister32(STATE_REGS_INPVOLUMERIGHT);
		m_extInputVolL = state.GetRegister32(STATE_REGS_EXTINPVOLUMELEFT);
		m_extInputVolR = state.GetRegister32(STATE_REGS_EXTINPVOLUMERIGHT);
		RegisterStateUtils::ReadArray(state, m_reverb, STATE_REGS_REVERB_FORMAT);
	}

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		auto& channel = m_channel[i];
		auto& reader = m_reader[i];

		auto channelRegsName = string_format(STATE_CHANNEL_REGS_FORMAT, i);
		const auto& channelState = stateCollectionFile.GetRegisterState(channelRegsName.c_str());

		channel.volumeLeft <<= channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMELEFT);
		channel.volumeRight <<= channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHT);
		channel.volumeLeftAbs = channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMELEFTABS);
		channel.volumeRightAbs = channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHTABS);
		channel.status = channelState.GetRegister32(STATE_CHANNEL_REGS_STATUS);
		channel.pitch = channelState.GetRegister32(STATE_CHANNEL_REGS_PITCH);
		channel.adsrLevel <<= channelState.GetRegister32(STATE_CHANNEL_REGS_ADSRLEVEL);
		channel.adsrRate <<= channelState.GetRegister32(STATE_CHANNEL_REGS_ADSRRATE);
		channel.adsrVolume = channelState.GetRegister32(STATE_CHANNEL_REGS_ADSRVOLUME);
		channel.address = channelState.GetRegister32(STATE_CHANNEL_REGS_ADDRESS);
		channel.repeat = channelState.GetRegister32(STATE_CHANNEL_REGS_REPEAT);
		channel.repeatSet = channelState.GetRegister32(STATE_CHANNEL_REGS_REPEATSET) != 0;
		channel.current = channelState.GetRegister32(STATE_CHANNEL_REGS_CURRENT);
		reader.LoadState(channelState);
	}
}

void CSpuBase::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto path = string_format(STATE_REGS_PATH_FORMAT, m_spuNumber);
	auto stateCollectionFile = std::make_unique<CRegisterStateCollectionFile>(path.c_str());

	{
		CRegisterState state;
		state.SetRegister32(STATE_REGS_CTRL, m_ctrl);
		state.SetRegister32(STATE_REGS_IRQADDR, m_irqAddr);
		state.SetRegister32(STATE_REGS_IRQPENDING, m_irqPending);
		state.SetRegister32(STATE_REGS_TRANSFERMODE, m_transferMode);
		state.SetRegister32(STATE_REGS_TRANSFERADDR, m_transferAddr);
		state.SetRegister32(STATE_REGS_CORE0OUTPUTOFFSET, m_core0OutputOffset);
		state.SetRegister32(STATE_REGS_CHANNELON, m_channelOn.f);
		state.SetRegister32(STATE_REGS_CHANNELREVERB, m_channelReverb.f);
		state.SetRegister32(STATE_REGS_REVERBWORKADDRSTART, m_reverbWorkAddrStart);
		state.SetRegister32(STATE_REGS_REVERBWORKADDREND, m_reverbWorkAddrEnd);
		state.SetRegister32(STATE_REGS_REVERBCURRADDR, m_reverbCurrAddr);
		state.SetRegister32(STATE_REGS_INPVOLUMELEFT, m_inputVolL);
		state.SetRegister32(STATE_REGS_INPVOLUMERIGHT, m_inputVolR);
		state.SetRegister32(STATE_REGS_EXTINPVOLUMELEFT, m_extInputVolL);
		state.SetRegister32(STATE_REGS_EXTINPVOLUMERIGHT, m_extInputVolR);
		RegisterStateUtils::WriteArray(state, m_reverb, STATE_REGS_REVERB_FORMAT);
		stateCollectionFile->InsertRegisterState(STATE_REGS, std::move(state));
	}

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		const auto& channel = m_channel[i];
		const auto& reader = m_reader[i];
		CRegisterState channelState;
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMELEFT, channel.volumeLeft);
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHT, channel.volumeRight);
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMELEFTABS, channel.volumeLeftAbs);
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHTABS, channel.volumeRightAbs);
		channelState.SetRegister32(STATE_CHANNEL_REGS_STATUS, channel.status);
		channelState.SetRegister32(STATE_CHANNEL_REGS_PITCH, channel.pitch);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADSRLEVEL, channel.adsrLevel);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADSRRATE, channel.adsrRate);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADSRVOLUME, channel.adsrVolume);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADDRESS, channel.address);
		channelState.SetRegister32(STATE_CHANNEL_REGS_REPEAT, channel.repeat);
		channelState.SetRegister32(STATE_CHANNEL_REGS_REPEATSET, channel.repeatSet);
		channelState.SetRegister32(STATE_CHANNEL_REGS_CURRENT, channel.current);
		reader.SaveState(channelState);

		auto channelRegsName = string_format(STATE_CHANNEL_REGS_FORMAT, i);
		stateCollectionFile->InsertRegisterState(channelRegsName.c_str(), std::move(channelState));
	}

	archive.InsertFile(std::move(stateCollectionFile));
}

bool CSpuBase::IsEnabled() const
{
	return (m_ctrl & 0x8000) != 0;
}

void CSpuBase::SetVolumeAdjust(float volumeAdjust)
{
	m_volumeAdjust = volumeAdjust;
}

void CSpuBase::SetReverbEnabled(bool enabled)
{
	m_reverbEnabled = enabled;
}

uint16 CSpuBase::GetControl() const
{
	return m_ctrl;
}

void CSpuBase::SetControl(uint16 value)
{
	m_ctrl = value;
	if((m_ctrl & CONTROL_IRQ) == 0)
	{
		ClearIrqPending();
		m_irqWatcher->ClearIrqPending(m_spuNumber);
	}
}

void CSpuBase::SetBaseSamplingRate(uint32 samplingRate)
{
	m_baseSamplingRate = samplingRate;
	m_blockReader.SetBaseSamplingRate(samplingRate);
}

void CSpuBase::SetInputBypass(bool inputBypass)
{
	m_blockReader.SetSpdifBypass(inputBypass);
}

void CSpuBase::SetDestinationSamplingRate(uint32 samplingRate)
{
	m_blockReader.SetDestinationSamplingRate(samplingRate);
	for(auto& reader : m_reader)
	{
		reader.SetDestinationSamplingRate(samplingRate);
	}
}

bool CSpuBase::GetIrqPending() const
{
	return m_irqPending;
}

void CSpuBase::ClearIrqPending()
{
	m_irqPending = false;
}

uint32 CSpuBase::GetIrqAddress() const
{
	return m_irqAddr;
}

void CSpuBase::SetIrqAddress(uint32 value)
{
	m_irqAddr = value & (m_ramSize - 1);
	m_irqWatcher->SetIrqAddress(m_spuNumber, m_irqAddr);
}

uint16 CSpuBase::GetTransferMode() const
{
	return m_transferMode;
}

void CSpuBase::SetTransferMode(uint16 transferMode)
{
	m_transferMode = transferMode;
}

uint32 CSpuBase::GetTransferAddress() const
{
	return m_transferAddr;
}

void CSpuBase::SetTransferAddress(uint32 value)
{
	m_transferAddr = value & (m_ramSize - 1);
}

UNION32_16 CSpuBase::GetChannelOn() const
{
	return m_channelOn;
}

void CSpuBase::SetChannelOnLo(uint16 value)
{
	m_channelOn.h0 = value;
}

void CSpuBase::SetChannelOnHi(uint16 value)
{
	m_channelOn.h1 = value;
}

UNION32_16 CSpuBase::GetChannelReverb() const
{
	return m_channelReverb;
}

void CSpuBase::SetChannelReverbLo(uint16 value)
{
	m_channelReverb.h0 = value;
}

void CSpuBase::SetChannelReverbHi(uint16 value)
{
	m_channelReverb.h1 = value;
}

uint32 CSpuBase::GetReverbParam(unsigned int param) const
{
	assert(param < REVERB_PARAM_COUNT);
	return m_reverb[param];
}

void CSpuBase::SetReverbParam(unsigned int param, uint32 value)
{
	assert(param < REVERB_PARAM_COUNT);
	m_reverb[param] = value;
}

UNION32_16 CSpuBase::GetEndFlags() const
{
	UNION32_16 result(0);
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		if(m_reader[i].GetEndFlag())
		{
			result.f |= (1 << i);
		}
	}
	return result;
}

void CSpuBase::ClearEndFlags()
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		m_reader[i].ClearEndFlag();
	}
}

CSpuBase::CHANNEL& CSpuBase::GetChannel(unsigned int channelNumber)
{
	assert(channelNumber < MAX_CHANNEL);
	return m_channel[channelNumber];
}

void CSpuBase::OnChannelPitchChanged(unsigned int channelNumber)
{
	assert(channelNumber < MAX_CHANNEL);
	auto& reader = m_reader[channelNumber];
	auto& channel = m_channel[channelNumber];
	reader.SetPitch(m_baseSamplingRate, channel.pitch);
}

void CSpuBase::SendKeyOn(uint32 channels)
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CHANNEL& channel = m_channel[i];
		if(channels & (1 << i))
		{
			channel.status = KEY_ON;
			channel.repeatSet = false;
			channel.current = channel.address;
			//Simulate one tick of ADSR in case game tries to read envelope value quickly
			//(ie.: Before 'Render' is called and while the voice is still in KEY_ON state)
			channel.adsrVolume = GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x10);
		}
	}
}

void CSpuBase::SendKeyOff(uint32 channels)
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CHANNEL& channel = m_channel[i];
		if(channels & (1 << i))
		{
			if(channel.status == STOPPED) continue;
			if(channel.status == KEY_ON)
			{
				channel.status = STOPPED;
				//Update channel registers since we are doing KEY_ON -> STOPPED transition
				auto& reader = m_reader[i];
				reader.SetParamsNoRead(channel.address, channel.repeat);
				reader.ClearEndFlag();
				channel.current = reader.GetCurrent();
			}
			else
			{
				channel.status = RELEASE;
			}
		}
	}
}

uint32 CSpuBase::GetReverbWorkAddressStart() const
{
	return m_reverbWorkAddrStart;
}

void CSpuBase::SetReverbWorkAddressStart(uint32 address)
{
	assert(address <= m_ramSize);
	m_reverbWorkAddrStart = address;
	m_reverbCurrAddr = address;
}

uint32 CSpuBase::GetReverbWorkAddressEnd() const
{
	return m_reverbWorkAddrEnd - 1;
}

void CSpuBase::SetReverbWorkAddressEnd(uint32 address)
{
	assert((address & 0xFFFF) == 0xFFFF);
	assert(address <= m_ramSize);
	m_reverbWorkAddrEnd = address + 1;
}

void CSpuBase::SetReverbCurrentAddress(uint32 address)
{
	m_reverbCurrAddr = address;
}

uint32 CSpuBase::ReceiveDma(uint8* buffer, uint32 blockSize, uint32 blockAmount, uint32 direction)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Receiving DMA transfer to 0x%08X. Size = 0x%08X bytes.\r\n",
	                          m_transferAddr, blockSize * blockAmount);
#endif
	if(m_transferMode == TRANSFER_MODE_VOICE)
	{
		if((m_ctrl & CONTROL_DMA) == CONTROL_DMA_STOP)
		{
			//Genso Suikoden 5 uses this
			return 0;
		}
		if((m_ctrl & CONTROL_DMA) == CONTROL_DMA_READ)
		{
			//- DMA reads need to be throttled to allow FFX IopSoundDriver to properly synchronize itself
			blockAmount = std::min<uint32>(blockAmount, 0x10);
			for(unsigned int i = 0; i < blockAmount; i++)
			{
				memcpy(buffer, m_ram + m_transferAddr, blockSize);
				m_transferAddr += blockSize;
				m_transferAddr &= m_ramSize - 1;
				buffer += blockSize;
			}
			return blockAmount;
		}
		//- Tsugunai needs voice transfers to be throttled because it starts a DMA transfer
		//  and then writes data that is necessary to the transfer callback in memory
		//- Some PSF sets (FF4, Xenogears, Xenosaga 2) are sensitive to aggressive throttling (doesn't like 0x10)
		blockAmount = std::min<uint32>(blockAmount, 0x100);
		assert((m_ctrl & CONTROL_DMA) == CONTROL_DMA_WRITE);
		unsigned int blocksTransfered = 0;
		m_sampleCache->ClearRange(m_transferAddr, blockSize * blockAmount);
		for(unsigned int i = 0; i < blockAmount; i++)
		{
			uint32 copySize = std::min<uint32>(m_ramSize - m_transferAddr, blockSize);
			memcpy(m_ram + m_transferAddr, buffer, copySize);
			m_transferAddr += blockSize;
			m_transferAddr &= m_ramSize - 1;
			buffer += blockSize;
			blocksTransfered++;
		}
		return blocksTransfered;
	}
	else if(
	    (m_transferMode == TRANSFER_MODE_BLOCK_CORE0IN) ||
	    (m_transferMode == TRANSFER_MODE_BLOCK_CORE1IN))
	{
		assert(m_transferAddr == 0);
		assert((m_spuNumber == 0) || !(m_transferMode == TRANSFER_MODE_BLOCK_CORE0IN));
		assert((m_spuNumber == 1) || !(m_transferMode == TRANSFER_MODE_BLOCK_CORE1IN));
		assert(m_blockWritePtr <= SOUND_INPUT_DATA_SIZE);

		uint32 availableBytes = SOUND_INPUT_DATA_SIZE - m_blockWritePtr;
		uint32 availableBlocks = availableBytes / blockSize;
		blockAmount = std::min(blockAmount, availableBlocks);

		uint32 dstAddr = m_soundInputDataAddr + m_blockWritePtr;
		memcpy(m_ram + dstAddr, buffer, blockAmount * blockSize);
		m_blockWritePtr += blockAmount * blockSize;

		return blockAmount;
	}
	else
	{
		return 1;
	}
}

void CSpuBase::WriteWord(uint16 value)
{
	assert((m_transferAddr + 1) < m_ramSize);
	*reinterpret_cast<uint16*>(&m_ram[m_transferAddr]) = value;
	m_sampleCache->ClearRange(m_transferAddr, 2);
	m_transferAddr += 2;
}

int32 CSpuBase::ComputeChannelVolume(const CHANNEL_VOLUME& volume, int32 currentVolume)
{
	int32 volumeLevel = 0;
	if(!volume.mode.mode)
	{
		if(volume.volume.phase)
		{
			volumeLevel = 0x3FFF - volume.volume.volume;
		}
		else
		{
			volumeLevel = volume.volume.volume;
		}
		volumeLevel <<= 17;
	}
	else
	{
		assert(volume.sweep.phase == 0);
		if(volume.sweep.slope == 0)
		{
			//Linear increase/decrease
			if(volume.sweep.decrease)
			{
				uint32 sweepDelta = g_linearDecreaseSweepDeltas[volume.sweep.volume];
				volumeLevel = currentVolume - sweepDelta;
			}
			else
			{
				uint32 sweepDelta = g_linearIncreaseSweepDeltas[volume.sweep.volume];
				volumeLevel = currentVolume + sweepDelta;
			}
		}
		else
		{
			//Exponential increase/decrease
			if(volume.sweep.decrease)
			{
				int64 sweepDelta = static_cast<int64>(currentVolume) * static_cast<int64>(volume.sweep.volume) / 0x7F;
				assert(sweepDelta >= 0);
				int32 baseVolume = std::max(1, currentVolume);
				uint32 sweepDeltaClamped = std::clamp<int64>(sweepDelta, 1, baseVolume);
				volumeLevel = baseVolume - sweepDeltaClamped;
			}
			else
			{
				//Not supported
				assert(false);
			}
		}
		volumeLevel = std::max<int32>(volumeLevel, 0x00000000);
		volumeLevel = std::min<int32>(volumeLevel, 0x7FFFFFFF);
	}
	return volumeLevel;
}

void CSpuBase::MixSamples(int32 inputSample, int32 volumeLevel, int16* output)
{
	inputSample = (inputSample * volumeLevel) / 0x7FFF;
	int32 resultSample = inputSample + static_cast<int32>(*output);
	resultSample = std::clamp<int32>(resultSample, SHRT_MIN, SHRT_MAX);

	*output = static_cast<int16>(resultSample);
}

void CSpuBase::Render(int16* samples, unsigned int sampleCount)
{
	bool updateReverb = m_reverbEnabled && (m_ctrl & CONTROL_REVERB) && (m_reverbWorkAddrStart < m_reverbWorkAddrEnd);
	bool irqEnabled = (m_ctrl & CONTROL_IRQ);

	int16* samplesBase = samples;
	assert((sampleCount & 0x01) == 0);
	unsigned int ticks = sampleCount / 2;
	memset(samples, 0, sizeof(int16) * sampleCount);

	for(unsigned int j = 0; j < ticks; j++)
	{
		int16 reverbSample[2] = {};
		//Update channels
		for(unsigned int i = 0; i < 24; i++)
		{
			auto& channel(m_channel[i]);
			auto& reader(m_reader[i]);
			if(channel.status == KEY_ON)
			{
				reader.SetParamsRead(channel.address, channel.repeat);
				reader.ClearEndFlag();
				channel.status = ATTACK;
				channel.adsrVolume = 0;
			}
			else
			{
				if(reader.IsDone())
				{
					channel.status = STOPPED;
					channel.adsrVolume = 0;
					reader.ClearIsDone();
				}
				if(reader.DidChangeRepeat() && !channel.repeatSet)
				{
					channel.repeat = reader.GetRepeat();
					reader.ClearDidChangeRepeat();
				}
				//Update repeat in case it has been changed externally (needed for FFX)
				reader.SetRepeat(channel.repeat);
			}

			int32 readSample = reader.GetSample();
			channel.current = reader.GetCurrent();

			UpdateAdsr(channel);
			channel.volumeLeftAbs = ComputeChannelVolume(channel.volumeLeft, channel.volumeLeftAbs);
			channel.volumeRightAbs = ComputeChannelVolume(channel.volumeRight, channel.volumeRightAbs);

			if(readSample == 0) continue;

			//Mix in adsrVolume
			int32 inputSample = (readSample * static_cast<int32>(channel.adsrVolume >> 16)) / static_cast<int32>(MAX_ADSR_VOLUME >> 16);

			if(inputSample == 0) continue;

			int32 volumeLeft = channel.volumeLeftAbs >> 16;
			int32 volumeRight = channel.volumeRightAbs >> 16;

			MixSamples(inputSample, volumeLeft, samples + 0);
			MixSamples(inputSample, volumeRight, samples + 1);

			//Mix in reverb if enabled for this channel
			if(updateReverb && (m_channelReverb.f & (1 << i)))
			{
				MixSamples(inputSample, volumeLeft, reverbSample + 0);
				MixSamples(inputSample, volumeRight, reverbSample + 1);
			}
		}

		if(!m_blockReader.CanReadSamples() && (m_blockWritePtr == SOUND_INPUT_DATA_SIZE))
		{
			//We're ready to consume some data
			m_blockReader.FillBlock(m_ram + m_soundInputDataAddr);
			m_blockWritePtr = 0;
		}

		if(m_blockReader.CanReadSamples())
		{
			int32 blockSamples[2] = {};
			m_blockReader.GetSamples(blockSamples);

			// Audio input data should have volume adjusted to BVOL register values . . .
			if(m_spuNumber == 0 && m_blockReader.GetSpdifBypass())
			{
				//  . . . unless in bypass mode
				MixSamples(blockSamples[0], 0x7FFF, samples + 0);
				MixSamples(blockSamples[1], 0x7FFF, samples + 1);
			}
			else
			{
				MixSamples(blockSamples[0], m_inputVolL, samples + 0);
				MixSamples(blockSamples[1], m_inputVolR, samples + 1);
			}
		}

		//Simulate SPU CORE0 writing its output in RAM and check for potential interrupts
		if(m_spuNumber == 0)
		{
			if(irqEnabled)
			{
				//TODO: Check which core is responsible for which area
				if(m_irqAddr == (CORE0_SIN_LEFT + m_core0OutputOffset))
				{
					m_irqPending = true;
				}
				else if(m_irqAddr == (CORE1_SIN_LEFT + m_core0OutputOffset))
				{
					m_irqPending = true;
				}
				else if(m_irqAddr == (CORE1_SIN_RIGHT + m_core0OutputOffset))
				{
					m_irqPending = true;
				}
			}
			m_core0OutputOffset += 2;
			m_core0OutputOffset &= (CORE0_OUTPUT_SIZE - 1);
		}

		//Update reverb
		if(updateReverb)
		{
			UpdateReverb(reverbSample, samples);
		}

		samples += 2;
	}

	if(irqEnabled && m_irqWatcher->HasPendingIrq(m_spuNumber))
	{
		m_irqPending = true;
	}
	m_irqWatcher->ClearIrqPending(m_spuNumber);

	if(m_volumeAdjust != 1.0f)
	{
		for(int i = 0; i < sampleCount; i++)
		{
			float adjustedSample = static_cast<float>(samplesBase[i]) * m_volumeAdjust;
			adjustedSample = std::clamp<float>(adjustedSample, SHRT_MIN, SHRT_MAX);
			samplesBase[i] = static_cast<int16>(adjustedSample);
		}
	}
}

uint32 CSpuBase::GetAdsrDelta(unsigned int index) const
{
	return m_adsrLogTable[index + 32];
}

float CSpuBase::GetReverbSample(uint32 address) const
{
	uint32 absoluteAddress = m_reverbCurrAddr + address;
	while(absoluteAddress >= m_reverbWorkAddrEnd)
	{
		absoluteAddress -= m_reverbWorkAddrEnd;
		absoluteAddress += m_reverbWorkAddrStart;
	}
	return static_cast<float>(*reinterpret_cast<int16*>(m_ram + absoluteAddress));
}

void CSpuBase::SetReverbSample(uint32 address, float value)
{
	uint32 absoluteAddress = m_reverbCurrAddr + address;
	while(absoluteAddress >= m_reverbWorkAddrEnd)
	{
		absoluteAddress -= m_reverbWorkAddrEnd;
		absoluteAddress += m_reverbWorkAddrStart;
	}
	value = std::max<float>(value, SHRT_MIN);
	value = std::min<float>(value, SHRT_MAX);
	int16 intValue = static_cast<int16>(value);
	*reinterpret_cast<int16*>(m_ram + absoluteAddress) = intValue;
}

uint32 CSpuBase::GetReverbOffset(unsigned int registerId) const
{
	return m_reverb[registerId];
}

float CSpuBase::GetReverbCoef(unsigned int registerId) const
{
	int16 value = static_cast<int16>(m_reverb[registerId]);
	return static_cast<float>(value) / static_cast<float>(0x8000);
}

void CSpuBase::UpdateAdsr(CHANNEL& channel)
{
	static const unsigned int logIndex[8] = {0, 4, 6, 8, 9, 10, 11, 12};
	int32 currentAdsrLevel = channel.adsrVolume;
	if(channel.status == ATTACK)
	{
		if(channel.adsrLevel.attackMode == 0)
		{
			//Linear mode
			currentAdsrLevel += GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x10);
		}
		else
		{
			if(currentAdsrLevel < 0x60000000)
			{
				currentAdsrLevel += GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x10);
			}
			else
			{
				currentAdsrLevel += GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x18);
			}
		}
		//Terminasion condition
		if(currentAdsrLevel < 0)
		{
			currentAdsrLevel = MAX_ADSR_VOLUME;
			channel.status = DECAY;
		}
	}
	else if(channel.status == DECAY)
	{
		unsigned int decayType = (static_cast<uint32>(currentAdsrLevel) >> 28) & 0x7;
		currentAdsrLevel -= GetAdsrDelta((4 * (channel.adsrLevel.decayRate ^ 0x1F)) - 0x18 + logIndex[decayType]);
		//Terminasion condition
		if(static_cast<unsigned int>((currentAdsrLevel >> 27) & 0xF) <= channel.adsrLevel.sustainLevel)
		{
			channel.status = SUSTAIN;
		}
	}
	else if(channel.status == SUSTAIN)
	{
		if(channel.adsrRate.sustainDirection == 0)
		{
			//Increment
			if(channel.adsrRate.sustainMode == 0)
			{
				currentAdsrLevel += GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x10);
			}
			else
			{
				if(currentAdsrLevel < 0x60000000)
				{
					currentAdsrLevel += GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x10);
				}
				else
				{
					currentAdsrLevel += GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x18);
				}
			}

			if(currentAdsrLevel < 0)
			{
				currentAdsrLevel = MAX_ADSR_VOLUME;
			}
		}
		else
		{
			//Decrement
			if(channel.adsrRate.sustainMode == 0)
			{
				//Linear
				currentAdsrLevel -= GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x0F);
			}
			else
			{
				unsigned int sustainType = (static_cast<uint32>(currentAdsrLevel) >> 28) & 0x7;
				currentAdsrLevel -= GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x1B + logIndex[sustainType]);
			}

			if(currentAdsrLevel < 0)
			{
				currentAdsrLevel = 0;
			}
		}
	}
	else if(channel.status == RELEASE)
	{
		if(channel.adsrRate.releaseMode == 0)
		{
			//Linear
			currentAdsrLevel -= GetAdsrDelta((4 * (channel.adsrRate.releaseRate ^ 0x1F)) - 0x0C);
		}
		else
		{
			unsigned int releaseType = (static_cast<uint32>(currentAdsrLevel) >> 28) & 0x7;
			currentAdsrLevel -= GetAdsrDelta((4 * (channel.adsrRate.releaseRate ^ 0x1F)) - 0x18 + logIndex[releaseType]);
		}

		if(currentAdsrLevel < 0)
		{
			currentAdsrLevel = 0;
			channel.status = STOPPED;
		}
	}
	channel.adsrVolume = static_cast<uint32>(currentAdsrLevel);
}

void CSpuBase::UpdateReverb(int16 reverbSample[2], int16* samples)
{
	//Feed samples to FIR filter
	if(m_reverbTicks & 1)
	{
		//IIR_INPUT_A0 = buffer[IIR_SRC_A0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
		//IIR_INPUT_A1 = buffer[IIR_SRC_A1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;
		//IIR_INPUT_B0 = buffer[IIR_SRC_B1] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
		//IIR_INPUT_B1 = buffer[IIR_SRC_B0] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;

		float input_sample_l = static_cast<float>(reverbSample[0]) * 0.5f;
		float input_sample_r = static_cast<float>(reverbSample[1]) * 0.5f;

		float irr_coef = GetReverbCoef(IIR_COEF);
		float in_coef_l = GetReverbCoef(IN_COEF_L);
		float in_coef_r = GetReverbCoef(IN_COEF_R);

		float iir_input_a0 = GetReverbSample(GetReverbOffset(IIR_SRC_A0)) * irr_coef + input_sample_l * in_coef_l;
		float iir_input_a1 = GetReverbSample(GetReverbOffset(IIR_SRC_A1)) * irr_coef + input_sample_r * in_coef_r;
		float iir_input_b0 = GetReverbSample(GetReverbOffset(IIR_SRC_B1)) * irr_coef + input_sample_l * in_coef_l;
		float iir_input_b1 = GetReverbSample(GetReverbOffset(IIR_SRC_B0)) * irr_coef + input_sample_r * in_coef_r;

		//IIR_A0 = IIR_INPUT_A0 * IIR_ALPHA + buffer[IIR_DEST_A0] * (1.0 - IIR_ALPHA);
		//IIR_A1 = IIR_INPUT_A1 * IIR_ALPHA + buffer[IIR_DEST_A1] * (1.0 - IIR_ALPHA);
		//IIR_B0 = IIR_INPUT_B0 * IIR_ALPHA + buffer[IIR_DEST_B0] * (1.0 - IIR_ALPHA);
		//IIR_B1 = IIR_INPUT_B1 * IIR_ALPHA + buffer[IIR_DEST_B1] * (1.0 - IIR_ALPHA);

		float iir_alpha = GetReverbCoef(IIR_ALPHA);

		float iir_a0 = iir_input_a0 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_A0)) * (1.0f - iir_alpha);
		float iir_a1 = iir_input_a1 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_A1)) * (1.0f - iir_alpha);
		float iir_b0 = iir_input_b0 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_B0)) * (1.0f - iir_alpha);
		float iir_b1 = iir_input_b1 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_B1)) * (1.0f - iir_alpha);

		//buffer[IIR_DEST_A0 + 1sample] = IIR_A0;
		//buffer[IIR_DEST_A1 + 1sample] = IIR_A1;
		//buffer[IIR_DEST_B0 + 1sample] = IIR_B0;
		//buffer[IIR_DEST_B1 + 1sample] = IIR_B1;

		SetReverbSample(GetReverbOffset(IIR_DEST_A0) + 2, iir_a0);
		SetReverbSample(GetReverbOffset(IIR_DEST_A1) + 2, iir_a1);
		SetReverbSample(GetReverbOffset(IIR_DEST_B0) + 2, iir_b0);
		SetReverbSample(GetReverbOffset(IIR_DEST_B1) + 2, iir_b1);

		//ACC0 = buffer[ACC_SRC_A0] * ACC_COEF_A +
		//	   buffer[ACC_SRC_B0] * ACC_COEF_B +
		//	   buffer[ACC_SRC_C0] * ACC_COEF_C +
		//	   buffer[ACC_SRC_D0] * ACC_COEF_D;
		//ACC1 = buffer[ACC_SRC_A1] * ACC_COEF_A +
		//	   buffer[ACC_SRC_B1] * ACC_COEF_B +
		//	   buffer[ACC_SRC_C1] * ACC_COEF_C +
		//	   buffer[ACC_SRC_D1] * ACC_COEF_D;

		float acc_coef_a = GetReverbCoef(ACC_COEF_A);
		float acc_coef_b = GetReverbCoef(ACC_COEF_B);
		float acc_coef_c = GetReverbCoef(ACC_COEF_C);
		float acc_coef_d = GetReverbCoef(ACC_COEF_D);

		float acc0 =
		    GetReverbSample(GetReverbOffset(ACC_SRC_A0)) * acc_coef_a +
		    GetReverbSample(GetReverbOffset(ACC_SRC_B0)) * acc_coef_b +
		    GetReverbSample(GetReverbOffset(ACC_SRC_C0)) * acc_coef_c +
		    GetReverbSample(GetReverbOffset(ACC_SRC_D0)) * acc_coef_d;

		float acc1 =
		    GetReverbSample(GetReverbOffset(ACC_SRC_A1)) * acc_coef_a +
		    GetReverbSample(GetReverbOffset(ACC_SRC_B1)) * acc_coef_b +
		    GetReverbSample(GetReverbOffset(ACC_SRC_C1)) * acc_coef_c +
		    GetReverbSample(GetReverbOffset(ACC_SRC_D1)) * acc_coef_d;

		//FB_A0 = buffer[MIX_DEST_A0 - FB_SRC_A];
		//FB_A1 = buffer[MIX_DEST_A1 - FB_SRC_A];
		//FB_B0 = buffer[MIX_DEST_B0 - FB_SRC_B];
		//FB_B1 = buffer[MIX_DEST_B1 - FB_SRC_B];

		float fb_a0 = GetReverbSample(GetReverbOffset(MIX_DEST_A0) - GetReverbOffset(FB_SRC_A));
		float fb_a1 = GetReverbSample(GetReverbOffset(MIX_DEST_A1) - GetReverbOffset(FB_SRC_A));
		float fb_b0 = GetReverbSample(GetReverbOffset(MIX_DEST_B0) - GetReverbOffset(FB_SRC_B));
		float fb_b1 = GetReverbSample(GetReverbOffset(MIX_DEST_B1) - GetReverbOffset(FB_SRC_B));

		//buffer[MIX_DEST_A0] = ACC0 - FB_A0 * FB_ALPHA;
		//buffer[MIX_DEST_A1] = ACC1 - FB_A1 * FB_ALPHA;
		//buffer[MIX_DEST_B0] = (FB_ALPHA * ACC0) - FB_A0 * (FB_ALPHA^0x8000) - FB_B0 * FB_X;
		//buffer[MIX_DEST_B1] = (FB_ALPHA * ACC1) - FB_A1 * (FB_ALPHA^0x8000) - FB_B1 * FB_X;

		float fb_alpha = GetReverbCoef(FB_ALPHA);
		float fb_x = GetReverbCoef(FB_X);

		SetReverbSample(GetReverbOffset(MIX_DEST_A0), acc0 - fb_a0 * fb_alpha);
		SetReverbSample(GetReverbOffset(MIX_DEST_A1), acc1 - fb_a1 * fb_alpha);
		SetReverbSample(GetReverbOffset(MIX_DEST_B0), (fb_alpha * acc0) - fb_a0 * -fb_alpha - fb_b0 * fb_x);
		SetReverbSample(GetReverbOffset(MIX_DEST_B1), (fb_alpha * acc1) - fb_a1 * -fb_alpha - fb_b1 * fb_x);

		m_reverbCurrAddr += 2;
		if(m_reverbCurrAddr >= m_reverbWorkAddrEnd)
		{
			m_reverbCurrAddr = m_reverbWorkAddrStart;
		}
	}

	if(m_reverbWorkAddrStart != 0)
	{
		float sampleL = 0.333f * (GetReverbSample(GetReverbOffset(MIX_DEST_A0)) + GetReverbSample(GetReverbOffset(MIX_DEST_B0)));
		float sampleR = 0.333f * (GetReverbSample(GetReverbOffset(MIX_DEST_A1)) + GetReverbSample(GetReverbOffset(MIX_DEST_B1)));

		{
			int16* output = samples + 0;
			int32 resultSample = static_cast<int32>(sampleL) + static_cast<int32>(*output);
			resultSample = std::max<int32>(resultSample, SHRT_MIN);
			resultSample = std::min<int32>(resultSample, SHRT_MAX);
			*output = static_cast<int16>(resultSample);
		}

		{
			int16* output = samples + 1;
			int32 resultSample = static_cast<int32>(sampleR) + static_cast<int32>(*output);
			resultSample = std::max<int32>(resultSample, SHRT_MIN);
			resultSample = std::min<int32>(resultSample, SHRT_MAX);
			*output = static_cast<int16>(resultSample);
		}
	}

	m_reverbTicks++;
}

///////////////////////////////////////////////////////
// CSpuSampleCache
///////////////////////////////////////////////////////

const CSpuSampleCache::ITEM* CSpuSampleCache::GetItem(const KEY& key) const
{
	auto range = m_cache.equal_range(key.address);
	for(auto itemIterator = range.first; itemIterator != range.second; itemIterator++)
	{
		const auto& item = itemIterator->second;
		if((item.inS1 == key.s1) && (item.inS2 == key.s2))
		{
			return &item;
		}
	}
	return nullptr;
}

CSpuSampleCache::ITEM& CSpuSampleCache::RegisterItem(const KEY& key)
{
	auto result = m_cache.emplace(std::make_pair(key.address, ITEM()));
	auto& item = result->second;
	item.inS1 = key.s1;
	item.inS2 = key.s2;
	return item;
}

void CSpuSampleCache::Clear()
{
	m_cache.clear();
}

void CSpuSampleCache::ClearRange(uint32 address, uint32 size)
{
	auto lowerBound = m_cache.lower_bound(address);
	auto upperBound = m_cache.upper_bound(address + size);
	m_cache.erase(lowerBound, upperBound);
}

///////////////////////////////////////////////////////
// CSpuIrqWatcher
///////////////////////////////////////////////////////

void CSpuIrqWatcher::Reset()
{
	for(int i = 0; i < MAX_CORES; i++)
	{
		m_irqPending[i] = false;
		m_irqAddr[i] = RESET_IRQ_ADDR;
	}
}

void CSpuIrqWatcher::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_IRQWATCHER_REGS_PATH));
	m_irqAddr[0] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQADDR0);
	m_irqAddr[1] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQADDR1);
	m_irqPending[0] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING0) != 0;
	m_irqPending[1] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING1) != 0;
}

void CSpuIrqWatcher::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_IRQWATCHER_REGS_PATH);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQADDR0, m_irqAddr[0]);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQADDR1, m_irqAddr[1]);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING0, m_irqPending[0]);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING1, m_irqPending[1]);
	archive.InsertFile(std::move(registerFile));
}

void CSpuIrqWatcher::SetIrqAddress(int core, uint32 address)
{
	m_irqAddr[core] = address;
}

void CSpuIrqWatcher::CheckIrq(uint32 address)
{
	for(int i = 0; i < MAX_CORES; i++)
	{
		if(address == m_irqAddr[i])
		{
			m_irqPending[i] = true;
		}
	}
}

void CSpuIrqWatcher::ClearIrqPending(int core)
{
	m_irqPending[core] = false;
}

bool CSpuIrqWatcher::HasPendingIrq(int core) const
{
	return m_irqPending[core];
}

///////////////////////////////////////////////////////
// CSampleReader
///////////////////////////////////////////////////////

CSpuBase::CSampleReader::CSampleReader()
{
	Reset();
}

void CSpuBase::CSampleReader::Reset()
{
	m_nextSampleAddr = 0;
	m_repeatAddr = 0;
	memset(m_buffer, 0, sizeof(m_buffer));
	m_pitch = 0;
	m_srcSampleIdx = 0;
	m_srcSamplingRate = 0;
	m_dstSamplingRate = 0;
	m_sampleStep = 0;
	m_s1 = 0;
	m_s2 = 0;
	m_done = false;
	m_didChangeRepeat = false;
	m_nextValid = false;
	m_endFlag = false;
}

void CSpuBase::CSampleReader::SetMemory(uint8* ram, uint32 ramSize)
{
	m_ram = ram;
	m_ramSize = ramSize;
	assert((ramSize & (ramSize - 1)) == 0);
}

void CSpuBase::CSampleReader::SetSampleCache(CSpuSampleCache* sampleCache)
{
	m_sampleCache = sampleCache;
}

void CSpuBase::CSampleReader::SetIrqWatcher(CSpuIrqWatcher* irqWatcher)
{
	m_irqWatcher = irqWatcher;
}

void CSpuBase::CSampleReader::SetDestinationSamplingRate(uint32 samplingRate)
{
	m_dstSamplingRate = samplingRate;
	UpdateSampleStep();
}

void CSpuBase::CSampleReader::LoadState(const CRegisterState& channelState)
{
	m_srcSampleIdx = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLEIDX);
	m_srcSamplingRate = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLINGRATE);
	m_nextSampleAddr = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_NEXTSAMPLEADDR);
	m_repeatAddr = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_REPEATADDR);
	m_pitch = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_PITCH);
	m_s1 = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_S1);
	m_s2 = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_S2);
	m_done = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_DONE) != 0;
	m_nextValid = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_NEXTVALID) != 0;
	m_endFlag = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_ENDFLAG) != 0;
	m_didChangeRepeat = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_DIDCHANGEREPEAT) != 0;
	RegisterStateUtils::ReadArray(channelState, m_buffer, STATE_SAMPLEREADER_REGS_BUFFER_FORMAT);

	UpdateSampleStep();
}

void CSpuBase::CSampleReader::SaveState(CRegisterState& channelState) const
{
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLEIDX, m_srcSampleIdx);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLINGRATE, m_srcSamplingRate);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_NEXTSAMPLEADDR, m_nextSampleAddr);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_REPEATADDR, m_repeatAddr);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_PITCH, m_pitch);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_S1, m_s1);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_S2, m_s2);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_DONE, m_done);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_NEXTVALID, m_nextValid);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_ENDFLAG, m_endFlag);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_DIDCHANGEREPEAT, m_didChangeRepeat);
	RegisterStateUtils::WriteArray(channelState, m_buffer, STATE_SAMPLEREADER_REGS_BUFFER_FORMAT);
}

void CSpuBase::CSampleReader::SetParams(uint32 address, uint32 repeat)
{
	m_srcSampleIdx = 0;
	m_nextSampleAddr = address & (m_ramSize - 1);
	m_repeatAddr = repeat & (m_ramSize - 1);
	m_s1 = 0;
	m_s2 = 0;
	m_nextValid = false;
	m_done = false;
	m_didChangeRepeat = false;
}

void CSpuBase::CSampleReader::SetParamsRead(uint32 address, uint32 repeat)
{
	SetParams(address, repeat);
	AdvanceBuffer();
}

void CSpuBase::CSampleReader::SetParamsNoRead(uint32 address, uint32 repeat)
{
	SetParams(address, repeat);
	memset(m_buffer, 0, sizeof(m_buffer));
}

void CSpuBase::CSampleReader::SetPitch(uint32 baseSamplingRate, uint16 pitch)
{
	m_srcSamplingRate = baseSamplingRate * pitch;
	UpdateSampleStep();
}

int32 CSpuBase::CSampleReader::GetSample()
{
	int16* sourceSignal = m_buffer + (m_srcSampleIdx >> 12);
	int16* gauss = (int16*) (((uint8*)g_gaussShuffledReverseTable) + ((m_srcSampleIdx & 0xFF0) >> 1));
	int32 resultSample;
	{ resultSample =
		(sourceSignal[0] * gauss[0]) +
		(sourceSignal[1] * gauss[1]) +
		(sourceSignal[2] * gauss[2]) +
		(sourceSignal[3] * gauss[3]);
	}
	resultSample >>= 15;
	m_srcSampleIdx += m_sampleStep;
	if(m_srcSampleIdx >= BUFFER_SAMPLES * PITCH_BASE)
	{
		m_srcSampleIdx -= BUFFER_SAMPLES * PITCH_BASE;
		AdvanceBuffer();
	}
	return resultSample;
}

void CSpuBase::CSampleReader::AdvanceBuffer()
{
	if(m_nextValid)
	{
		memmove(m_buffer, m_buffer + BUFFER_SAMPLES, sizeof(int16) * 4);
		UnpackSamples(m_buffer + 4);
	}
	else
	{
		memset(m_buffer, 0, sizeof(int16) * 4);
		UnpackSamples(m_buffer + 4);
		m_nextValid = true;
	}
}

void CSpuBase::CSampleReader::UpdateSampleStep()
{
	m_sampleStep = m_srcSamplingRate / m_dstSamplingRate;
}

void CSpuBase::CSampleReader::UnpackSamples(int16* dst)
{
	int32 workBuffer[BUFFER_SAMPLES];

	const uint8* nextSample = m_ram + m_nextSampleAddr;

	m_irqWatcher->CheckIrq(m_nextSampleAddr);

	//Read header
	uint8 shiftFactor = nextSample[0] & 0xF;
	uint8 predictNumber = nextSample[0] >> 4;
	uint8 flags = nextSample[1];
	assert(predictNumber < 5);

	auto cacheKey = CSpuSampleCache::KEY{m_nextSampleAddr, m_s1, m_s2};
	if(predictNumber == 0)
	{
		cacheKey.s1 = 0;
		cacheKey.s2 = 0;
	}

	if(auto cacheItem = m_sampleCache->GetItem(cacheKey))
	{
		memcpy(dst, cacheItem->samples, sizeof(int16) * BUFFER_SAMPLES);
		m_s1 = cacheItem->outS1;
		m_s2 = cacheItem->outS2;
	}
	else
	{
		//Get intermediate values
		{
			unsigned int workBufferPtr = 0;
			for(unsigned int i = 2; i < 16; i++)
			{
				uint8 sampleByte = nextSample[i];
				int16 firstSample = ((sampleByte & 0x0F) << 12);
				int16 secondSample = ((sampleByte & 0xF0) << 8);
				firstSample >>= shiftFactor;
				secondSample >>= shiftFactor;
				workBuffer[workBufferPtr++] = firstSample;
				workBuffer[workBufferPtr++] = secondSample;
			}
		}

		//Generate PCM samples
		{
			// clang-format off
			//Table is 16 entries long to prevent reading indeterminate
			//values if predictNumber is greater or equal to 5.
			//According to some sources, entries at 5 and beyond contain 0 on real hardware
			static const int32 predictorTable[16][2] =
			{
				{0, 0},
				{60, 0},
				{115, -52},
				{98, -55},
				{122, -60},
			};
			// clang-format on

			for(unsigned int i = 0; i < BUFFER_SAMPLES; i++)
			{
				int32 currentValue = workBuffer[i] * 64;
				currentValue += (m_s1 * predictorTable[predictNumber][0]) / 64;
				currentValue += (m_s2 * predictorTable[predictNumber][1]) / 64;
				m_s2 = m_s1;
				m_s1 = currentValue;
				int32 result = (currentValue + 32) / 64;
				result = std::max<int32>(result, SHRT_MIN);
				result = std::min<int32>(result, SHRT_MAX);
				dst[i] = static_cast<int16>(result);
			}

			auto& cacheItem = m_sampleCache->RegisterItem(cacheKey);
			memcpy(&cacheItem.samples, dst, sizeof(int16) * BUFFER_SAMPLES);
			cacheItem.outS1 = m_s1;
			cacheItem.outS2 = m_s2;
		}
	}

	if(flags & 0x04)
	{
		m_repeatAddr = m_nextSampleAddr;
		m_didChangeRepeat = true;
	}

	m_nextSampleAddr += 0x10;
	assert(m_nextSampleAddr < m_ramSize);
	m_nextSampleAddr &= (m_ramSize - 1);

	if(flags & 0x01)
	{
		m_endFlag = true;
		m_nextSampleAddr = m_repeatAddr;

		//If flags is in { 0x01, 0x05, 0x07 }, mute channel (Xenogears requires that)
		if(flags != 0x03)
		{
			m_done = true;
		}
	}
}

uint32 CSpuBase::CSampleReader::GetRepeat() const
{
	return m_repeatAddr;
}

void CSpuBase::CSampleReader::SetRepeat(uint32 repeatAddr)
{
	m_repeatAddr = repeatAddr & (m_ramSize - 1);
}

uint32 CSpuBase::CSampleReader::GetCurrent() const
{
	//Simulate a kind of progress inside the current sample address.
	//Doesn't need to be accurate, but it needs to change. Needed by Romancing Saga.
	uint32 intraSampleIdx = std::min<uint32>((m_srcSampleIdx / PITCH_BASE) / 2, 0x0E);
	return m_nextSampleAddr + intraSampleIdx;
}

bool CSpuBase::CSampleReader::IsDone() const
{
	return m_done;
}

void CSpuBase::CSampleReader::ClearIsDone()
{
	m_done = false;
}

bool CSpuBase::CSampleReader::GetEndFlag() const
{
	return m_endFlag;
}

void CSpuBase::CSampleReader::ClearEndFlag()
{
	m_endFlag = false;
}

bool CSpuBase::CSampleReader::DidChangeRepeat() const
{
	return m_didChangeRepeat;
}

void CSpuBase::CSampleReader::ClearDidChangeRepeat()
{
	m_didChangeRepeat = false;
}

///////////////////////////////////////////////////////
// CBlockSampleReader
///////////////////////////////////////////////////////

void CSpuBase::CBlockSampleReader::Reset()
{
	m_baseSamplingRate = INIT_SAMPLE_RATE;
	m_dstSamplingRate = 0;
	m_srcSampleIdx = SOUND_INPUT_DATA_SAMPLES * TIME_SCALE;
	m_sampleStep = 0;
}

void CSpuBase::CBlockSampleReader::SetBaseSamplingRate(uint32 baseSamplingRate)
{
	m_baseSamplingRate = baseSamplingRate;
	UpdateSampleStep();
}

void CSpuBase::CBlockSampleReader::SetDestinationSamplingRate(uint32 dstSamplingRate)
{
	m_dstSamplingRate = dstSamplingRate;
	UpdateSampleStep();
}

bool CSpuBase::CBlockSampleReader::GetSpdifBypass()
{
	return m_spdifBypass;
}

void CSpuBase::CBlockSampleReader::SetSpdifBypass(bool spdifBypass)
{
	m_spdifBypass = spdifBypass;
}

bool CSpuBase::CBlockSampleReader::CanReadSamples() const
{
	uint32 sampleIdx = (m_srcSampleIdx / TIME_SCALE);
	if(m_spdifBypass)
	{
		// in S/PDIF bypass mode, we consume 2 adjacent samples per cycle
		// as a stereo pair.
		return (sampleIdx < (SOUND_INPUT_DATA_SAMPLES / 2));
	}
	else
	{
		return (sampleIdx < SOUND_INPUT_DATA_SAMPLES);
	}
}

void CSpuBase::CBlockSampleReader::FillBlock(const uint8* block)
{
	memcpy(m_blockBuffer, block, SOUND_INPUT_DATA_SIZE);
	m_srcSampleIdx = 0;
}

void CSpuBase::CBlockSampleReader::GetSamples(int32 samples[2])
{
	assert(m_sampleStep != 0);

	uint32 srcSampleIdx = m_srcSampleIdx / TIME_SCALE;
	assert(srcSampleIdx < SOUND_INPUT_DATA_SAMPLES);

	auto inputSamples = reinterpret_cast<const int16*>(m_blockBuffer);

	if(m_spdifBypass)
	{
		// in S/PDIF bypass mode, input data contains linear interleaved
		// stereo samples, in a single stream. The L sample should always
		// be at an even-numbered index.
		samples[0] = inputSamples[0x000 + (srcSampleIdx * 2)];
		samples[1] = inputSamples[0x000 + (srcSampleIdx * 2) + 1];
	}
	else
	{
		samples[0] = inputSamples[0x000 + srcSampleIdx];
		samples[1] = inputSamples[0x100 + srcSampleIdx];
	}

	m_srcSampleIdx += m_sampleStep;
}

void CSpuBase::CBlockSampleReader::UpdateSampleStep()
{
	m_sampleStep = (m_baseSamplingRate * TIME_SCALE) / m_dstSamplingRate;
}
