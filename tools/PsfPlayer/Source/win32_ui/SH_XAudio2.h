#pragma once

#include <windows.h>
#include <xaudio2.h>
#include "../SoundHandler.h"
#include "win32/ComPtr.h"

class CSH_XAudio2 : public CSoundHandler
{
public:
											CSH_XAudio2();
	virtual									~CSH_XAudio2();

	static CSoundHandler*					HandlerFactory();

	void									Reset();
	bool									HasFreeBuffers();
	void									RecycleBuffers();
	void									Write(int16*, unsigned int, unsigned int);

private:
	typedef Framework::Win32::CComPtr<IClassFactory> ClassFactoryPtr;

	class VoiceCallback : public IXAudio2VoiceCallback
	{
	public:
		VoiceCallback(CSH_XAudio2* parent)
			: m_parent(parent)
		{

		}

		virtual ~VoiceCallback() {}

		STDMETHOD_(void, OnStreamEnd) () {}
		STDMETHOD_(void, OnVoiceProcessingPassEnd) () {}
		STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) {}
		STDMETHOD_(void, OnBufferEnd) (void* context) { m_parent->OnBufferEnd(context); }
		STDMETHOD_(void, OnBufferStart) (void*) {}
		STDMETHOD_(void, OnLoopEnd) (void*) {}
		STDMETHOD_(void, OnVoiceError) (void*, HRESULT) {}

	private:
		CSH_XAudio2* m_parent;
	};

	enum
	{
		MAX_BUFFERS = 25,
	};

	struct BUFFERINFO
	{
		bool		inUse;
		uint8*		data;
		uint32		dataSize;
	};

	void									InitializeXAudio2();

	BUFFERINFO*								GetFreeBuffer();

	void									OnBufferEnd(void*);

	ClassFactoryPtr							m_classFactory;
	Framework::Win32::CComPtr<IXAudio2>		m_xaudio2;
	IXAudio2MasteringVoice*					m_masteringVoice;
	IXAudio2SourceVoice*					m_sourceVoice;
	VoiceCallback*							m_voiceCallback;

	BUFFERINFO								m_buffers[MAX_BUFFERS];
};
