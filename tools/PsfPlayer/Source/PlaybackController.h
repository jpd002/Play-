#pragma once

#include "signal/Signal.h"
#include "PsfTags.h"

class CPlaybackController
{
public:
	void Play(const CPsfTags&);
	void Tick();

	uint64 GetFrameCount() const;

	Framework::CSignal<void()> PlaybackCompleted;
	Framework::CSignal<void(float)> VolumeChanged;

private:
	bool m_isPlaying = false;
	float m_volumeAdjust = 1.0f;
	uint64 m_frames = 0;
	uint64 m_fadePosition = 0;
	uint64 m_trackLength = 0;
};
