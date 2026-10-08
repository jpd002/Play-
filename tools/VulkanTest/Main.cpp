#include <cstdio>
#include <stdexcept>
#include "AppConfig.h"
#include "gs/GSH_Vulkan/GSH_VulkanOffscreen.h"
#include "gs/GSH_Vulkan/GSH_VulkanDeviceInfo.h"
#include "gs/GsPixelFormats.h"
#include "vulkan/StructDefs.h"

fs::path CAppConfig::GetBasePath() const
{
	return fs::current_path();
}

static void Verify(bool condition, const char* message)
{
	if(!condition) throw std::runtime_error(message);
}

class CTestHandler : public CGSH_VulkanOffscreen
{
public:
	explicit CTestHandler(bool useSurface = false)
	    : m_useSurface(useSurface)
	{
	}

	void Present(uint32 format)
	{
		SendGSCall([&]() {
			SyncMemoryCache();
			DISPLAY_INFO info;
			info.width = 64;
			info.height = 64;
			info.layers[0].enabled = true;
			info.layers[0].width = 64;
			info.layers[0].height = 64;
			info.layers[0].bufWidth = 64;
			info.layers[0].psm = format;
			FlipImpl(info);
			m_context->device.vkQueueWaitIdle(m_context->queue);
		}, true);
	}
	void Write(uint8 reg, uint64 value)
	{
		SendGSCall([this, reg, value]() { WriteRegisterImpl(reg, value); }, true);
	}

	void ChangeScale(int scale)
	{
		CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_RESOLUTION_FACTOR, scale);
		SendGSCall([this]() { NotifyPreferencesChangedImpl(); }, true);
	}

	uint32 GetMaxScale() const
	{
		return m_context->maxFramebufferScale;
	}

	std::string GetDeviceName() const
	{
		VkPhysicalDeviceProperties properties = {};
		m_instance.vkGetPhysicalDeviceProperties(m_context->physicalDevice, &properties);
		return properties.deviceName;
	}

	std::vector<uint8> ReadMemory()
	{
		std::vector<uint8> memory;
		SendGSCall([&]() {
			SyncMemoryCache();
			memory.assign(GetRam(), GetRam() + m_context->GetMemorySize());
		}, true);
		return memory;
	}

	void Transfer(uint32 format, const std::vector<uint8>& data)
	{
		SendGSCall([&]() {
			SyncMemoryCache();
			auto commands = std::make_shared<GSH_Vulkan::CFrameCommandBuffer>(m_context);
			GSH_Vulkan::CTransferHost host(m_context, commands);
			commands->RegisterWriter(&host);
			commands->BeginFrame();
			auto caps = make_convertible<GSH_Vulkan::CTransferHost::PIPELINE_CAPS>(0);
			caps.dstFormat = format;
			host.SetPipelineCaps(caps);
			host.Params.bufAddress = 0x80000;
			host.Params.bufWidth = 64;
			host.Params.rrw = 4;
			host.DoTransfer(data);
			commands->EndFrame();
			m_context->device.vkQueueWaitIdle(m_context->queue);
		}, true);
	}

	void CopyTransfer(uint32 format)
	{
		SendGSCall([&]() {
			SyncMemoryCache();
			auto commands = std::make_shared<GSH_Vulkan::CFrameCommandBuffer>(m_context);
			GSH_Vulkan::CTransferLocal local(m_context, commands);
			commands->BeginFrame();
			auto caps = make_convertible<GSH_Vulkan::CTransferLocal::PIPELINE_CAPS>(0);
			caps.srcFormat = format;
			caps.dstFormat = format;
			local.SetPipelineCaps(caps);
			local.Params.srcBufAddress = 0x80000;
			local.Params.dstBufAddress = 0xA0000;
			local.Params.srcBufWidth = 64;
			local.Params.dstBufWidth = 64;
			local.Params.rrw = 4;
			local.Params.rrh = 1;
			local.DoTransfer();
			commands->EndFrame();
			m_context->device.vkQueueWaitIdle(m_context->queue);
		}, true);
	}

protected:
	void InitializeImpl() override
	{
		m_instance = CreateInstance(true);
#ifdef _WIN32
		if(m_useSurface)
		{
			//Exercise the swapchain/presentation shaders without showing a window.
			m_window = CreateWindowExW(0, L"STATIC", L"VulkanTest", WS_POPUP, 0, 0, 128, 128,
			                           nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
			auto surface = Framework::Vulkan::Win32SurfaceCreateInfoKHR();
			surface.hwnd = m_window;
			surface.hinstance = GetModuleHandleW(nullptr);
			auto result = m_instance.vkCreateWin32SurfaceKHR(m_instance, &surface, nullptr, &m_context->surface);
			CHECKVULKANERROR(result);
		}
#endif
		CGSH_Vulkan::InitializeImpl();
	}

	void ReleaseImpl() override
	{
		CGSH_Vulkan::ReleaseImpl();
#ifdef _WIN32
		if(m_window)
		{
			m_instance.vkDestroySurfaceKHR(m_instance, m_context->surface, nullptr);
			m_context->surface = VK_NULL_HANDLE;
			DestroyWindow(m_window);
			m_window = nullptr;
		}
#endif
	}

private:
	bool m_useSurface = false;
#ifdef _WIN32
	HWND m_window = nullptr;
#endif
};

static CGSHandler::FRAME SetupDraw(CTestHandler& gs, uint32 format)
{
	auto frame = make_convertible<CGSHandler::FRAME>(0);
	frame.nWidth = 1;
	frame.nPsm = format;
	auto depth = make_convertible<CGSHandler::ZBUF>(0);
	depth.nPtr = 128;
	auto test = make_convertible<CGSHandler::TEST>(0);
	test.nDepthEnabled = 1;
	test.nDepthMethod = CGSHandler::DEPTH_TEST_ALWAYS;
	auto scissor = make_convertible<CGSHandler::SCISSOR>(0);
	scissor.scax1 = 63;
	scissor.scay1 = 63;
	gs.Write(GS_REG_FRAME_1, frame);
	gs.Write(GS_REG_ZBUF_1, depth);
	gs.Write(GS_REG_TEST_1, test);
	gs.Write(GS_REG_SCISSOR_1, scissor);
	gs.Write(GS_REG_PRMODECONT, 1);
	gs.Write(GS_REG_RGBAQ, 0x3F800000FFFFFFFFULL);
	gs.Write(GS_REG_COLCLAMP, 1);
	return frame;
}

static void DrawTriangle(CTestHandler& gs)
{
	gs.Write(GS_REG_PRIM, CGSHandler::PRIM_TRIANGLE);
	for(auto pos : {std::pair<uint32, uint32>{68, 68}, {628, 68}, {68, 516}})
	{
		auto xyz = make_convertible<CGSHandler::XYZ>(0);
		xyz.nX = pos.first;
		xyz.nY = pos.second;
		xyz.nZ = 0x40000000;
		gs.Write(GS_REG_XYZ2, xyz);
	}
}

static void TestRendering(CTestHandler& gs, int scale, uint32 format)
{
	gs.ChangeScale(scale);
	gs.Reset();
	auto frame = SetupDraw(gs, format);
	DrawTriangle(gs);
	auto image = gs.GetFramebuffer(frame);
	auto depth = make_convertible<CGSHandler::ZBUF>(0);
	depth.nPtr = 128;
	auto depthImage = gs.GetDepthbuffer(frame, depth);
	Verify(image.GetWidth() == 64 * scale && image.GetHeight() == 1024 * scale, "Framebuffer dimensions do not match scale");
	Verify(depthImage.GetWidth() == image.GetWidth(), "Depth dimensions do not match scale");
	Verify(image.GetPixel(10 * scale, 10 * scale).r != 0, "Triangle did not render");
	Verify(image.GetPixel(60 * scale, 60 * scale).r == 0, "Triangle escaped its bounds");
	for(uint32 y = 0; y < 64 * scale; y++)
	{
		for(uint32 x = 0; x < 64 * scale; x++)
		{
			Verify((image.GetPixel(x, y).r != 0) == (depthImage.GetPixel(x, y).a != 0), "Color and depth sample coverage differ");
		}
	}
	if(scale > 1)
	{
		uint32 mixedPixels = 0;
		for(uint32 y = 0; y < 64; y++)
		{
			for(uint32 x = 0; x < 64; x++)
			{
				auto first = image.GetPixel(x * scale, y * scale).r;
				for(int sy = 0; sy < scale; sy++)
				{
					for(int sx = 0; sx < scale; sx++)
					{
						if(image.GetPixel(x * scale + sx, y * scale + sy).r != first) mixedPixels++;
					}
				}
			}
		}
		Verify(mixedPixels > 0, "High resolution output is only an enlarged native image");
	}
	std::printf("Rendering x%d, PSM %u passed\n", scale, format);
}

static void TestRenderToTexture(CTestHandler& gs, int scale)
{
	gs.Reset();
	auto frame = SetupDraw(gs, CGSHandler::PSMCT32);
	DrawTriangle(gs);
	auto source = gs.GetFramebuffer(frame);
	frame.nPtr = 16;
	gs.Write(GS_REG_FRAME_1, frame);
	auto tex = make_convertible<CGSHandler::TEX0>(0);
	tex.nBufWidth = 1;
	tex.nWidth = 6;
	tex.nPad0 = 2;
	tex.nPad1 = 1;
	tex.nColorComp = 1;
	tex.nFunction = CGSHandler::TEX0_FUNCTION_DECAL;
	gs.Write(GS_REG_TEX0_1, tex);
	auto prim = make_convertible<CGSHandler::PRIM>(0);
	prim.nType = CGSHandler::PRIM_SPRITE;
	prim.nTexture = 1;
	prim.nUseUV = 1;
	gs.Write(GS_REG_PRIM, prim);
	gs.Write(GS_REG_UV, 0);
	gs.Write(GS_REG_XYZ2, 0x4000000000000000ULL);
	gs.Write(GS_REG_UV, uint64(64 * 16) | (uint64(64 * 16) << 16));
	gs.Write(GS_REG_XYZ2, 0x4000000000000000ULL | uint64(64 * 16) | (uint64(64 * 16) << 16));
	auto output = gs.GetFramebuffer(frame);
	//Check the sloped edge survives a framebuffer texture read at subpixel precision.
	uint32 differences = 0;
	for(uint32 y = 2 * scale; y < 62 * scale; y++)
	{
		for(uint32 x = 2 * scale; x < 62 * scale; x++)
		{
			if(source.GetPixel(x, y).r != output.GetPixel(x, y).r) differences++;
		}
	}
	Verify(differences == 0, "Render-to-texture lost high resolution samples");
	std::printf("Render-to-texture x%d passed\n", scale);
}

template <typename Indexor>
static void TestTransfer(CTestHandler& gs, uint32 format, uint32 bytesPerPixel, uint32 mask)
{
	std::vector<uint8> data;
	for(uint32 pixel = 0; pixel < 4; pixel++)
	{
		uint32 value = (0x87654321 + pixel) & mask;
		if(bytesPerPixel == 0)
		{
			if((pixel & 1) == 0) data.push_back(value | (((0x87654322 + pixel) & mask) << 4));
		}
		else
		{
			for(uint32 byte = 0; byte < bytesPerPixel; byte++) data.push_back(value >> (byte * 8));
		}
	}
	gs.Transfer(format, data);
	gs.CopyTransfer(format);
	auto memory = gs.ReadMemory();
	uint32 scale = gs.GetFramebufferScale();
	for(uint32 sample = 0; sample < scale * scale; sample++)
	{
		auto plane = memory.data() + sample * CGSHandler::RAMSIZE;
		Indexor src(plane, 0x80000, 1);
		Indexor dst(plane, 0xA0000, 1);
		for(uint32 x = 0; x < 4; x++)
		{
			uint32 expected = (0x87654321 + x) & mask;
			Verify((src.GetPixel(x, 0) & mask) == expected, "Host transfer lost a sample");
			Verify((dst.GetPixel(x, 0) & mask) == expected, "Local transfer lost a sample");
		}
	}
	std::printf("Transfer PSM %u passed\n", format);
}

int main()
{
	try
	{
		CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CGSHANDLER_RESOLUTION_FACTOR, 1);
		CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_RESOLUTION_FACTOR, 1);
		Verify(GSH_Vulkan::CDeviceInfo::GetInstance().HasAvailableDevices(), "No compatible Vulkan device");
		CTestHandler gs(true);
		gs.Initialize();
		std::printf("Device: %s\n", gs.GetDeviceName().c_str());
		try
		{
			Verify(gs.GetMaxScale() >= 2, "Device does not support x2");
			for(int scale : {1, 2, 4})
			{
				if(scale > gs.GetMaxScale()) continue;
				for(uint32 format : {CGSHandler::PSMCT32, CGSHandler::PSMCT24, CGSHandler::PSMCT16, CGSHandler::PSMCT16S})
				{
					TestRendering(gs, scale, format);
					gs.Present(format);
				}
				TestRenderToTexture(gs, scale);
				TestTransfer<CGsPixelFormats::CPixelIndexorPSMCT32>(gs, CGSHandler::PSMCT32, 4, ~0U);
				TestTransfer<CGsPixelFormats::CPixelIndexorPSMCT32>(gs, CGSHandler::PSMCT24, 3, 0xFFFFFF);
				TestTransfer<CGsPixelFormats::CPixelIndexorPSMCT16>(gs, CGSHandler::PSMCT16, 2, 0xFFFF);
				TestTransfer<CGsPixelFormats::CPixelIndexorPSMT8>(gs, CGSHandler::PSMT8, 1, 0xFF);
				TestTransfer<CGsPixelFormats::CPixelIndexorPSMT4>(gs, CGSHandler::PSMT4, 0, 0xF);
			}
			auto before = gs.ReadMemory();
			gs.ChangeScale(1);
			Verify(gs.GetFramebufferScale() == 1, "Could not return to native resolution");
			auto after = gs.ReadMemory();
			Verify(std::equal(after.begin(), after.end(), before.begin()), "Changing resolution destroyed native VRAM");
			gs.ChangeScale(-1);
			Verify(gs.GetFramebufferScale() == 1, "Invalid resolution setting was not clamped");
		}
		catch(...)
		{
			gs.Release();
			throw;
		}
		gs.Release();
		std::puts("Vulkan tests passed");
		return 0;
	}
	catch(const std::exception& e)
	{
		std::fprintf(stderr, "%s\n", e.what());
		return 1;
	}
}
