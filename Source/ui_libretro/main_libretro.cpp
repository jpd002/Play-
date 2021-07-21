#include "libretro.h"

#include "Log.h"
#include "AppConfig.h"
#include "PS2VM.h"
#include "DiskUtils.h"

#include "PS2VM_Preferences.h"
#include "GSH_OpenGL_Libretro.h"
#include "SH_LibreAudio.h"
#include "PH_Libretro_Input.h"

#include "PathUtils.h"
#include "PtrStream.h"
#include "MemStream.h"

#include "filesystem_def.h"
#include <vector>
#include <cstdlib>

#define LOG_NAME "LIBRETRO"

static CPS2VM* m_virtualMachine = nullptr;
static bool first_run = false;

bool libretro_supports_bitmasks = false;
retro_video_refresh_t g_video_cb;
retro_environment_t g_environ_cb;
retro_input_poll_t g_input_poll_cb;
retro_input_state_t g_input_state_cb;
retro_audio_sample_batch_t g_set_audio_sample_batch_cb;

std::map<int, int> g_ds2_to_retro_btn_map;
struct retro_hw_render_callback g_hw_render
{
};

int g_res_factor = 1;
CGSHandler::PRESENTATION_MODE g_presentation_mode = CGSHandler::PRESENTATION_MODE::PRESENTATION_MODE_FIT;
bool g_forceBilinearTextures = false;

static std::vector<struct retro_variable> m_vars =
    {
        {"play_res_multi", "Resolution Multiplier; 1x|2x|4x|8x"},
        {"play_presentation_mode", "Presentation Mode; Fit Screen|Fill Screen|Original Size"},
        {"play_bilinear_filtering", "Force Bilinear Filtering; false|true"},
        {NULL, NULL},
};

enum BootType
{
	CD,
	ELF
};

struct LastOpenCommand
{
	LastOpenCommand() = default;
	LastOpenCommand(BootType type, fs::path path)
	    : type(type)
	    , path(path)
	{
	}
	BootType type = BootType::CD;
	fs::path path;
};

LastOpenCommand m_bootCommand;

unsigned retro_api_version()
{
	return RETRO_API_VERSION;
}

void SetupVideoHandler()
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	auto gsHandler = m_virtualMachine->GetGSHandler();
	if(!gsHandler)
	{
		m_virtualMachine->CreateGSHandler(CGSH_OpenGL_Libretro::GetFactoryFunction());
	}
	else
	{
		auto retro_gs = static_cast<CGSH_OpenGL_Libretro*>(gsHandler);
		retro_gs->Reset();
	}
}

static void retro_context_destroy()
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
}

static void retro_context_reset()
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(m_virtualMachine)
	{
		SetupVideoHandler();
	}
}

void SetupSoundHandler()
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(m_virtualMachine)
	{
		m_virtualMachine->CreateSoundHandler(&CSH_LibreAudio::HandlerFactory);
	}
}

void SetupInputHandler()
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(!m_virtualMachine->GetPadHandler())
	{
		static struct retro_input_descriptor descDS2[] =
		    {
		        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Stick X"},
		        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Stick Y"},
		        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Stick X"},
		        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Stick Y"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Square"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Triangle"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Circle"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Cross"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L1"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R1"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2"},
		        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3"},
		        {0},
		    };

		g_environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, descDS2);

		static const struct retro_controller_description controllers[] = {
		    {"PS2 DualShock2", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
		};

		static const struct retro_controller_info ports[] = {
		    {controllers, 1},
		    {NULL, 0},
		};

		g_environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

		for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
		{
			auto ds2_button = static_cast<PS2::CControllerInfo::BUTTON>(i);
			auto retro_button = descDS2[i].id;
			g_ds2_to_retro_btn_map[ds2_button] = retro_button;
		}

		m_virtualMachine->CreatePadHandler(CPH_Libretro_Input::GetFactoryFunction());
	}
}

void retro_get_system_info(struct retro_system_info* info)
{
	*info = {};
	info->library_name = "Play!";
	info->library_version = PLAY_VERSION;
	info->need_fullpath = true;
	info->valid_extensions = "elf|iso|cso|isz|cue|chd";
}

void retro_get_system_av_info(struct retro_system_av_info* info)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	*info = {};
	info->timing.fps = 60.0;
	info->timing.sample_rate = 44100;
	info->geometry.base_width = 640;
	info->geometry.base_height = 448;
	info->geometry.max_width = 640 * 8;
	info->geometry.max_height = 448 * 8;
	info->geometry.aspect_ratio = 4.0 / 3.0;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
	g_video_cb = cb;
}

void retro_set_environment(retro_environment_t cb)
{
	g_environ_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
	g_input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
	g_input_state_cb = cb;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
	g_set_audio_sample_batch_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
}

unsigned retro_get_region(void)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	return RETRO_REGION_NTSC;
}

size_t retro_serialize_size(void)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	return 40 * 1024 * 1024;
}

bool retro_serialize(void* data, size_t size)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	try
	{
		Framework::CMemStream stateStream;
		Framework::CZipArchiveWriter archive;

		m_virtualMachine->m_ee->SaveState(archive);
		m_virtualMachine->m_iop->SaveState(archive);
		m_virtualMachine->m_ee->m_gs->SaveState(archive);

		archive.Write(stateStream);
		stateStream.Seek(0, Framework::STREAM_SEEK_DIRECTION::STREAM_SEEK_SET);
		stateStream.Read(data, size);
	}
	catch(...)
	{
		return false;
	}

	return true;
}

bool retro_unserialize(const void* data, size_t size)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	try
	{
		Framework::CPtrStream stateStream(data, size);
		Framework::CZipArchiveReader archive(stateStream);

		try
		{
			m_virtualMachine->m_ee->LoadState(archive);
			m_virtualMachine->m_iop->LoadState(archive);
			m_virtualMachine->m_ee->m_gs->LoadState(archive);
		}
		catch(...)
		{
			//Any error that occurs in the previous block is critical
			throw;
		}
	}
	catch(...)
	{
		return false;
	}

	m_virtualMachine->OnMachineStateChange();
	return true;
}

void* retro_get_memory_data(unsigned id)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(id == RETRO_MEMORY_SYSTEM_RAM)
	{
		return m_virtualMachine->m_ee->m_ram;
	}
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(id == RETRO_MEMORY_SYSTEM_RAM)
	{
		return PS2::EE_RAM_SIZE;
	}
	return 0;
}

void retro_cheat_reset(void)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
}

void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	(void)index;
	(void)enabled;
	(void)code;
}

void updateVars()
{
	for(int i = 0; i < m_vars.size() - 1; ++i)
	{
		auto item = m_vars[i];
		if(!item.key)
			continue;

		struct retro_variable var = {nullptr};
		var.key = item.key;
		if(g_environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			bool videoUpdate = false;
			switch(i)
			{
			case 0:
			{
				std::string val = var.value;
				auto res_factor = std::atoi(val.substr(0, -1).c_str());
				if(res_factor != g_res_factor)
				{
					g_res_factor = res_factor;
					CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSH_OPENGL_RESOLUTION_FACTOR, res_factor);
					videoUpdate = true;
				}
			}
			break;
			case 1:
			{
				CGSHandler::PRESENTATION_MODE presentation_mode = CGSHandler::PRESENTATION_MODE::PRESENTATION_MODE_FIT;

				std::string val(var.value);
				if(val == "Fill Screen")
					presentation_mode = CGSHandler::PRESENTATION_MODE::PRESENTATION_MODE_FILL;
				else if(val == "Original Size")
					presentation_mode = CGSHandler::PRESENTATION_MODE::PRESENTATION_MODE_ORIGINAL;

				if(presentation_mode != g_presentation_mode)
				{
					g_presentation_mode = presentation_mode;
					CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, presentation_mode);
					videoUpdate = true;
				}
			}
			break;
			case 2:
			{
				bool forceBilinearTextures = (std::string(var.value) == "true");
				if(forceBilinearTextures != g_forceBilinearTextures)
				{
					g_forceBilinearTextures = forceBilinearTextures;
					CAppConfig::GetInstance().SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, forceBilinearTextures);
					videoUpdate = true;
				}
			}
			break;
			}

			if(videoUpdate)
			{
				if(m_virtualMachine)
					if(m_virtualMachine->GetGSHandler())
						static_cast<CGSH_OpenGL_Libretro*>(m_virtualMachine->GetGSHandler())->UpdatePresentation();
			}
		}
	}
}

void checkVarsUpdates()
{
	static bool updates = true;
	if(!updates)
		g_environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updates);

	if(updates)
	{
		updateVars();
	}
	updates = false;
}

void retro_run()
{
	// CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	checkVarsUpdates();

	if(!first_run)
	{
		if(m_virtualMachine)
		{
			// m_virtualMachine->Pause();
			m_virtualMachine->Reset();
			if(m_bootCommand.type == BootType::CD)
			{
				m_virtualMachine->m_ee->m_os->BootFromCDROM();
			}
			else
			{
				m_virtualMachine->m_ee->m_os->BootFromFile(m_bootCommand.path);
			}
			m_virtualMachine->Resume();
			first_run = true;
			CLog::GetInstance().Print(LOG_NAME, "%s\n", "Start Game");
		}
	}

	if(m_virtualMachine)
	{
		auto pad = m_virtualMachine->GetPadHandler();
		if(pad)
			static_cast<CPH_Libretro_Input*>(pad)->UpdateInputState();

		if(m_virtualMachine->GetSoundHandler())
			static_cast<CSH_LibreAudio*>(m_virtualMachine->GetSoundHandler())->ProcessBuffer();

		if(m_virtualMachine->GetGSHandler())
			m_virtualMachine->GetGSHandler()->ProcessSingleFrame();
	}
}

void retro_reset(void)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(m_virtualMachine)
	{
		if(!m_virtualMachine->GetGSHandler())
			SetupVideoHandler();
		// m_virtualMachine->Pause();
		m_virtualMachine->Reset();
		m_virtualMachine->m_ee->m_os->BootFromCDROM();
		m_virtualMachine->Resume();
		CLog::GetInstance().Print(LOG_NAME, "%s\n", "Reset Game");
	}
	first_run = false;
}

bool IsBootableExecutablePath(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".elf");
}

bool IsBootableDiscImagePath(const fs::path& filePath)
{
	const auto& supportedExtensions = DiskUtils::GetSupportedExtensions();
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	auto extensionIterator = supportedExtensions.find(extension);
	return extensionIterator != std::end(supportedExtensions);
}

bool retro_load_game(const retro_game_info* info)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	fs::path filePath = info->path;
	if(IsBootableExecutablePath(filePath))
	{
		m_bootCommand = LastOpenCommand(BootType::ELF, filePath);
	}
	else if(IsBootableDiscImagePath(filePath))
	{
		m_bootCommand = LastOpenCommand(BootType::CD, filePath);
		CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, filePath);
		CAppConfig::GetInstance().Save();
	}
	first_run = false;

	auto rgb = RETRO_PIXEL_FORMAT_XRGB8888;
	g_environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb);

#ifdef GLES_COMPATIBILITY
	g_hw_render.context_type = RETRO_HW_CONTEXT_OPENGLES3;
#else
	g_hw_render.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
#endif

	g_hw_render.version_major = 3;
	g_hw_render.version_minor = 2;
	g_hw_render.context_reset = retro_context_reset;
	g_hw_render.context_destroy = retro_context_destroy;
	g_hw_render.cache_context = false;
	g_hw_render.bottom_left_origin = true;
	g_hw_render.depth = true;
	g_environ_cb(RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT, nullptr);

	g_environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &g_hw_render);

	g_environ_cb(RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT, nullptr);

	g_environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)m_vars.data());

	return true;
}

void retro_unload_game(void)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info)
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	return false;
}

void retro_init()
{
#ifdef __ANDROID__
	Framework::PathUtils::SetFilesDirPath(getenv("EXTERNAL_STORAGE"));
#endif
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(g_environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
		libretro_supports_bitmasks = true;

	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_AUDIO_SPUBLOCKCOUNT, 22);

	m_virtualMachine = new CPS2VM();
	m_virtualMachine->Initialize();

	//Disable frame limiter, RetroArch handles this on its own
	CAppConfig::GetInstance().SetPreferenceBoolean(PREF_PS2_LIMIT_FRAMERATE, false);
	m_virtualMachine->ReloadFrameRateLimit();

	SetupInputHandler();
	SetupSoundHandler();
	first_run = false;
}

void retro_deinit()
{
	CLog::GetInstance().Print(LOG_NAME, "%s\n", __FUNCTION__);

	if(m_virtualMachine)
	{
		m_virtualMachine->PauseAsync();
		auto gsHandler = static_cast<CGSH_OpenGL_Libretro*>(m_virtualMachine->GetGSHandler());
		if(gsHandler)
		{
			// Note: since we've forced GS into running on this/main/libretro thread
			// we need to clear its queue, to prevent it from locking up VM
			while(m_virtualMachine->GetStatus() != CVirtualMachine::PAUSED)
			{
				std::this_thread::yield();
				gsHandler->Release();
			}
		}
		m_virtualMachine->DestroyPadHandler();
		m_virtualMachine->DestroyGSHandler();
		m_virtualMachine->DestroySoundHandler();
		m_virtualMachine->Destroy();
		delete m_virtualMachine;
		m_virtualMachine = nullptr;
	}
	libretro_supports_bitmasks = false;
}
