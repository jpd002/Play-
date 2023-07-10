#include "PlaybackController.h"
#include <string>

void CPlaybackController::Play(const CPsfTags& tags)
{
	m_volumeAdjust = 1.0f;
	try
	{
		m_volumeAdjust = stof(tags.GetTagValue("volume"));
	}
	catch(...)
	{
	}
	if(tags.HasTag("length"))
	{
		double length = CPsfTags::ConvertTimeString(tags.GetTagValue("length").c_str());
		m_trackLength = static_cast<uint64>(length * 60.0);
	}
	else
	{
		m_trackLength = 3600; //1 minute default length
	}
	double fade = 10.f; //10 seconds default fade
	if(tags.HasTag("fade"))
	{
		fade = CPsfTags::ConvertTimeString(tags.GetTagValue("fade").c_str());
	}
	m_fadePosition = m_trackLength;
	m_trackLength += static_cast<uint64>(fade * 60.f);
	m_frames = 0;
	m_isPlaying = true;

	VolumeChanged(m_volumeAdjust);
}

void CPlaybackController::Tick()
{
	if(!m_isPlaying)
	{
		return;
	}
	m_frames++;
	if(m_frames > m_trackLength)
	{
		m_isPlaying = false;
		PlaybackCompleted();
	}
	else if(m_frames >= m_fadePosition)
	{
		float currentRatio = static_cast<float>(m_frames - m_fadePosition) / static_cast<float>(m_trackLength - m_fadePosition);
		float currentVolume = (1.0f - currentRatio) * m_volumeAdjust;
		VolumeChanged(currentVolume);
	}
}

uint64 CPlaybackController::GetFrameCount() const
{
	return m_frames;
}
