#include "SH_OpenAL.h"
#include <cassert>

#ifdef __EMSCRIPTEN__
#include <emscripten/em_js.h>

EM_JS(void, js_init_webaudio, (int sampleRate), {
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

    // Ring buffer: 131,072 stereo frames capacity (~3.0s at 44.1kHz)
    const RING_SIZE = 131072;
    const RING_MASK = RING_SIZE - 1;
    globalThis.__ps2AudioL = new Float32Array(RING_SIZE);
    globalThis.__ps2AudioR = new Float32Array(RING_SIZE);
    globalThis.__ps2AudioReadPtr = 0;
    globalThis.__ps2AudioWritePtr = 0;
    globalThis.__ps2InSampleRate = sampleRate || 44100;
    globalThis.__ps2AudioStarted = false;

    let fracPos = 0.0;
    let lastL = 0.0;
    let lastR = 0.0;

    const resumeAudio = () => {
        if (ctx && ctx.state === 'suspended') {
            ctx.resume();
        }
    };
    ['click', 'keydown', 'touchstart', 'mousedown', 'pointerdown'].forEach(ev => {
        window.addEventListener(ev, resumeAudio, { once: false, passive: true });
        document.addEventListener(ev, resumeAudio, { once: false, passive: true });
    });

    // ScriptProcessor with buffer size 1024 (~21ms @ 48kHz)
    const processor = ctx.createScriptProcessor(1024, 0, 2);
    processor.onaudioprocess = (e) => {
        if (ctx.state === 'suspended') {
            ctx.resume();
        }

        const outL = e.outputBuffer.getChannelData(0);
        const outR = e.outputBuffer.getChannelData(1);
        const numFrames = outL.length;

        let rPtr = globalThis.__ps2AudioReadPtr;
        const wPtr = globalThis.__ps2AudioWritePtr;
        const bufL = globalThis.__ps2AudioL;
        const bufR = globalThis.__ps2AudioR;
        const inRate = globalThis.__ps2InSampleRate || 44100;
        const outRate = ctx.sampleRate || 44100;
        const baseStep = inRate / outRate;

        // Lock-free single-producer single-consumer available calculation
        let available = (wPtr - rPtr) & RING_MASK;

        // Pre-buffering: wait until ~120ms (5,000 frames) is buffered before starting playback
        if (!globalThis.__ps2AudioStarted) {
            if (available >= 5000) {
                globalThis.__ps2AudioStarted = true;
            } else {
                for (let i = 0; i < numFrames; i++) {
                    outL[i] = 0.0;
                    outR[i] = 0.0;
                }
                return;
            }
        }

        // Bound maximum buffer backlog to ~500ms (24,000 frames) to prevent delay drift
        if (available > 24000) {
            const drop = available - 8000;
            rPtr = (rPtr + drop) & RING_MASK;
            available = 8000;
        }

        // Continuous rate adaptation: smoothly tracks emulator speed to prevent buffer underruns
        const TARGET_BUFFER = 6000;
        let targetRate = 1.0;
        if (available < TARGET_BUFFER) {
            targetRate = Math.max(0.5, Math.min(1.0, (available / TARGET_BUFFER)));
        } else {
            targetRate = Math.min(1.2, 1.0 + (available - TARGET_BUFFER) * 0.00002);
        }

        if (typeof globalThis.__ps2CurrentRateAdjust !== 'number') {
            globalThis.__ps2CurrentRateAdjust = 1.0;
        }
        globalThis.__ps2CurrentRateAdjust += (targetRate - globalThis.__ps2CurrentRateAdjust) * 0.08;
        const step = baseStep * globalThis.__ps2CurrentRateAdjust;

        for (let i = 0; i < numFrames; i++) {
            const currentAvail = (wPtr - rPtr) & RING_MASK;
            if (currentAvail >= 2) {
                const idx0 = rPtr;
                const idx1 = (rPtr + 1) & RING_MASK;
                const s0L = bufL[idx0];
                const s1L = bufL[idx1];
                const s0R = bufR[idx0];
                const s1R = bufR[idx1];

                // Linear interpolation resampling
                lastL = s0L + (s1L - s0L) * fracPos;
                lastR = s0R + (s1R - s0R) * fracPos;
                outL[i] = lastL;
                outR[i] = lastR;

                fracPos += step;
                while (fracPos >= 1.0) {
                    fracPos -= 1.0;
                    rPtr = (rPtr + 1) & RING_MASK;
                }
            } else {
                // Smooth fade out on rare underrun
                lastL *= 0.98;
                lastR *= 0.98;
                outL[i] = lastL;
                outR[i] = lastR;
            }
        }

        globalThis.__ps2AudioReadPtr = rPtr;
    };

    processor.connect(ctx.destination);
    globalThis.__ps2AudioProcessor = processor;
});

EM_JS(void, js_write_webaudio_samples, (const int16_t* ptr, int count, int sampleRate), {
    if (!globalThis.__ps2AudioInitialized) {
        return;
    }
    const ctx = globalThis.__ps2AudioCtx;
    if (ctx && ctx.state === 'suspended') {
        ctx.resume();
    }

    const RING_SIZE = 131072;
    const RING_MASK = RING_SIZE - 1;
    const bufL = globalThis.__ps2AudioL;
    const bufR = globalThis.__ps2AudioR;
    let wPtr = globalThis.__ps2AudioWritePtr;

    // Stereo frame count: count is the total number of 16-bit words (L0, R0, L1, R1...)
    const numFrames = count >> 1;
    const heapIdx = ptr >> 1;
    const heap = HEAP16;

    for (let i = 0; i < numFrames; i++) {
        const idx = heapIdx + (i << 1);
        bufL[wPtr] = heap[idx] * (1.0 / 32768.0);
        bufR[wPtr] = heap[idx + 1] * (1.0 / 32768.0);
        wPtr = (wPtr + 1) & RING_MASK;
    }

    globalThis.__ps2AudioWritePtr = wPtr;
    globalThis.__ps2InSampleRate = sampleRate;
});

EM_JS(void, js_reset_webaudio, (), {
    globalThis.__ps2AudioReadPtr = 0;
    globalThis.__ps2AudioWritePtr = 0;
    globalThis.__ps2AudioStarted = false;
});

CSH_OpenAL::CSH_OpenAL()
{
	js_init_webaudio(44100);
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
	js_reset_webaudio();
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
	js_write_webaudio_samples(samples, sampleCount, sampleRate);
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
