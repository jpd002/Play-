#include "SH_OpenAL.h"
#include <cassert>

#ifdef __EMSCRIPTEN__
#include <emscripten/em_js.h>
#include <cstdint>

#pragma pack(push, 4)
struct AudioRingBuffer
{
	uint32_t readPtr;
	uint32_t writePtr;
	uint32_t sampleRate;
	uint32_t started;
	uint32_t padding[12]; // 64 bytes total header
	float audioData[65536 * 2]; // 65,536 stereo frames (512 KB)
};
#pragma pack(pop)

static AudioRingBuffer s_audioRingBuffer;

EM_JS(void, js_init_webaudio, (uintptr_t ringPtr, int defaultSampleRate), {
    if (ENVIRONMENT_IS_PTHREAD) return;
    if (globalThis.__ps2AudioInitialized) return;
    globalThis.__ps2AudioInitialized = true;

    const AudioContextClass = globalThis.AudioContext || globalThis.webkitAudioContext;
    if (!AudioContextClass) return;

    let ctx;
    try {
        ctx = new AudioContextClass({ latencyHint: 'interactive' });
    } catch (e) {
        ctx = new AudioContextClass();
    }
    globalThis.__ps2AudioCtx = ctx;

    const resumeAudio = () => {
        if (ctx && ctx.state === 'suspended') {
            ctx.resume();
        }
    };
    ['click', 'keydown', 'touchstart', 'mousedown', 'pointerdown', 'focus'].forEach(ev => {
        window.addEventListener(ev, resumeAudio, { once: false, passive: true });
        document.addEventListener(ev, resumeAudio, { once: false, passive: true });
    });

    let cachedRingBuffer = null;
    let header = null;
    let audioData = null;
    const getRing = () => {
        if (cachedRingBuffer !== wasmMemory.buffer) {
            cachedRingBuffer = wasmMemory.buffer;
            header = new Int32Array(wasmMemory.buffer, ringPtr, 16);
            audioData = new Float32Array(wasmMemory.buffer, ringPtr + 64, 65536 * 2);
        }
        return { header, audioData };
    };

    if (defaultSampleRate && defaultSampleRate > 8000) {
        const ring = getRing();
        Atomics.store(ring.header, 2, defaultSampleRate);
    }

    let fracPos = 0.0;
    let lastL = 0.0;
    let lastR = 0.0;
    let currentRate = 1.0;
    const TARGET_BUFFER = 12288;
    const HIST_LEN = 1024;
    const histL = new Float32Array(HIST_LEN);
    const histR = new Float32Array(HIST_LEN);
    let histPtr = 0;
    let decayGain = 1.0;
    let concealIdx = 0;

    const processor = ctx.createScriptProcessor(2048, 0, 2);
    processor.onaudioprocess = (e) => {
        if (ctx.state === 'suspended') {
            ctx.resume();
        }

        const outL = e.outputBuffer.getChannelData(0);
        const outR = e.outputBuffer.getChannelData(1);
        const numFrames = outL.length;

        const ring = getRing();
        const header = ring.header;
        const audioData = ring.audioData;

        let rPtr = Atomics.load(header, 0);
        let wPtr = Atomics.load(header, 1);
        const inRate = Atomics.load(header, 2) || 44100;
        const outRate = ctx.sampleRate || 44100;
        const baseStep = inRate / outRate;

        let available = (wPtr - rPtr) & 65535;
        let started = Atomics.load(header, 3);

        if (!started) {
            if (available >= TARGET_BUFFER) {
                Atomics.store(header, 3, 1);
                started = 1;
            } else {
                for (let i = 0; i < numFrames; i++) {
                    outL[i] = 0.0;
                    outR[i] = 0.0;
                }
                return;
            }
        }

        if (available > 32768) {
            const drop = available - TARGET_BUFFER;
            rPtr = (rPtr + drop) & 65535;
            available = TARGET_BUFFER;
        }

        const rateAdj = Math.max(-0.008, Math.min(0.008, (available - TARGET_BUFFER) / 32768.0));
        const targetRate = 1.0 + rateAdj;
        currentRate = currentRate * 0.92 + targetRate * 0.08;
        const step = baseStep * currentRate;

        for (let i = 0; i < numFrames; i++) {
            let currentAvail = (wPtr - rPtr) & 65535;
            if (currentAvail < 2) {
                wPtr = Atomics.load(header, 1);
                currentAvail = (wPtr - rPtr) & 65535;
            }

            if (currentAvail >= 2) {
                const idx0 = (rPtr << 1);
                const idx1 = (((rPtr + 1) & 65535) << 1);
                const s0L = audioData[idx0];
                const s1L = audioData[idx1];
                const s0R = audioData[idx0 + 1];
                const s1R = audioData[idx1 + 1];
                lastL = s0L + (s1L - s0L) * fracPos;
                lastR = s0R + (s1R - s0R) * fracPos;
                outL[i] = lastL;
                outR[i] = lastR;
                histL[histPtr] = lastL;
                histR[histPtr] = lastR;
                histPtr = (histPtr + 1) & (HIST_LEN - 1);
                decayGain = 1.0;
                concealIdx = 0;
                fracPos += step;
                while (fracPos >= 1.0) {
                    fracPos -= 1.0;
                    rPtr = (rPtr + 1) & 65535;
                }
            } else {
                concealIdx = (concealIdx + 1) & 511;
                const fetchIdx = (histPtr - 512 + concealIdx) & (HIST_LEN - 1);
                decayGain = Math.max(0.0, decayGain * 0.998);
                outL[i] = histL[fetchIdx] * decayGain;
                outR[i] = histR[fetchIdx] * decayGain;
            }
        }

        Atomics.store(header, 0, rPtr);
    };

    processor.connect(ctx.destination);
    globalThis.__ps2AudioProcessor = processor;
});

CSH_OpenAL::CSH_OpenAL()
{
	js_init_webaudio(reinterpret_cast<uintptr_t>(&s_audioRingBuffer), 44100);
}

CSH_OpenAL::~CSH_OpenAL()
{
	Reset();
}

CSoundHandler* CSH_OpenAL::HandlerFactory()
{
	return new CSH_OpenAL();
}

void CSH_OpenAL::Reset()
{
	__atomic_store_n(&s_audioRingBuffer.readPtr, 0, __ATOMIC_RELEASE);
	__atomic_store_n(&s_audioRingBuffer.writePtr, 0, __ATOMIC_RELEASE);
	__atomic_store_n(&s_audioRingBuffer.started, 0, __ATOMIC_RELEASE);
}

void CSH_OpenAL::RecycleBuffers()
{
}

bool CSH_OpenAL::HasFreeBuffers()
{
	return true;
}

uint32 CSH_OpenAL::GetFreeBufferCount() const
{
	return MAX_BUFFERS;
}

void CSH_OpenAL::Write(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	uint32_t wPtr = __atomic_load_n(&s_audioRingBuffer.writePtr, __ATOMIC_ACQUIRE);
	const unsigned int numFrames = sampleCount >> 1;
	for(unsigned int i = 0; i < numFrames; i++)
	{
		uint32_t dstIdx = (wPtr << 1);
		s_audioRingBuffer.audioData[dstIdx] = samples[i * 2] * (1.0f / 32768.0f);
		s_audioRingBuffer.audioData[dstIdx + 1] = samples[i * 2 + 1] * (1.0f / 32768.0f);
		wPtr = (wPtr + 1) & 65535;
	}
	__atomic_store_n(&s_audioRingBuffer.writePtr, wPtr, __ATOMIC_RELEASE);
	if(sampleRate > 8000)
	{
		__atomic_store_n(&s_audioRingBuffer.sampleRate, sampleRate, __ATOMIC_RELEASE);
	}
}

#else

#include "alloca_def.h"

#define SAMPLE_RATE 44100

ALCint g_attrList[] =
    {
        ALC_FREQUENCY, SAMPLE_RATE,
        0, 0};

#define CHECK_AL_ERROR()                     \
	{                                        \
		assert(alGetError() == AL_NO_ERROR); \
	}

CSH_OpenAL::CSH_OpenAL()
    : m_context(m_device, g_attrList)
    , m_lastUpdateTime(0)
    , m_mustSync(true)
{
	m_context.MakeCurrent();
	alGenBuffers(MAX_BUFFERS, m_bufferNames);
	CHECK_AL_ERROR();
	Reset();
}

CSH_OpenAL::~CSH_OpenAL()
{
	Reset();
	alDeleteBuffers(MAX_BUFFERS, m_bufferNames);
	CHECK_AL_ERROR();
}

CSoundHandler* CSH_OpenAL::HandlerFactory()
{
	return new CSH_OpenAL();
}

void CSH_OpenAL::Reset()
{
	m_source.Stop();
	ALint sourceState = m_source.GetState();
	assert(sourceState == AL_INITIAL || sourceState == AL_STOPPED);
	alSourcei(m_source, AL_BUFFER, 0);
	CHECK_AL_ERROR();
	m_availableBuffers.clear();
	m_availableBuffers.insert(m_availableBuffers.begin(), m_bufferNames, m_bufferNames + MAX_BUFFERS);
}

void CSH_OpenAL::RecycleBuffers()
{
	unsigned int bufferCount = m_source.GetBuffersProcessed();
	CHECK_AL_ERROR();
	if(bufferCount != 0)
	{
		ALuint* bufferNames = reinterpret_cast<ALuint*>(alloca(sizeof(ALuint) * bufferCount));
		alSourceUnqueueBuffers(m_source, bufferCount, bufferNames);
		CHECK_AL_ERROR();
		m_availableBuffers.insert(m_availableBuffers.begin(), bufferNames, bufferNames + bufferCount);
	}
}

bool CSH_OpenAL::HasFreeBuffers()
{
	return m_availableBuffers.size() != 0;
}

uint32 CSH_OpenAL::GetFreeBufferCount() const
{
	return m_availableBuffers.size();
}

void CSH_OpenAL::Write(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	RecycleBuffers();

	if(m_availableBuffers.size() == 0) return;
	ALuint buffer = *m_availableBuffers.begin();
	m_availableBuffers.pop_front();

	alBufferData(buffer, AL_FORMAT_STEREO16, samples, sampleCount * sizeof(int16), sampleRate);
	CHECK_AL_ERROR();

	alSourceQueueBuffers(m_source, 1, &buffer);
	CHECK_AL_ERROR();

	ALint sourceState = m_source.GetState();
	if(sourceState != AL_PLAYING)
	{
		m_source.Play();
		assert(m_source.GetState() == AL_PLAYING);
	}
}

#endif
