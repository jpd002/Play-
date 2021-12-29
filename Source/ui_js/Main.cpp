#include <cstdio>
#include <emscripten/bind.h>
#include "Ps2VmJs.h"
#include "GSH_OpenGLJs.h"
#include "PS2VM_Preferences.h"
#include "AppConfig.h"
#include "input/PH_GenericInput.h"
#include "InputProviderEmscripten.h"

CPs2VmJs* g_virtualMachine = nullptr;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_context = 0;
std::shared_ptr<CInputProviderEmscripten> g_inputProvider;

int main(int argc, const char** argv)
{
	return 0;
}

EM_BOOL keyboardCallback(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData)
{
	if(keyEvent->repeat)
	{
		return true;
	}
	switch(eventType)
	{
	case EMSCRIPTEN_EVENT_KEYDOWN:
		g_inputProvider->OnKeyDown(keyEvent->code);
		break;
	case EMSCRIPTEN_EVENT_KEYUP:
		g_inputProvider->OnKeyUp(keyEvent->code);
		break;
	}
	return true;
}

extern "C" void initVm()
{
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.majorVersion = 2;
	attr.minorVersion = 0;
	attr.alpha = false;
	g_context = emscripten_webgl_create_context("#outputCanvas", &attr);
	assert(g_context >= 0);

	g_virtualMachine = new CPs2VmJs();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreateGSHandler(CGSH_OpenGLJs::GetFactoryFunction(g_context));

	{
		g_virtualMachine->CreatePadHandler(CPH_GenericInput::GetFactoryFunction());
		auto padHandler = static_cast<CPH_GenericInput*>(g_virtualMachine->GetPadHandler());
		auto& bindingManager = padHandler->GetBindingManager();

		g_inputProvider = std::make_shared<CInputProviderEmscripten>();
		bindingManager.RegisterInputProvider(g_inputProvider);

		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::START, CInputProviderEmscripten::MakeBindingTarget("Enter"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::SELECT, CInputProviderEmscripten::MakeBindingTarget("Backspace"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::DPAD_LEFT, CInputProviderEmscripten::MakeBindingTarget("ArrowLeft"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::DPAD_RIGHT, CInputProviderEmscripten::MakeBindingTarget("ArrowRight"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::DPAD_UP, CInputProviderEmscripten::MakeBindingTarget("ArrowUp"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::DPAD_DOWN, CInputProviderEmscripten::MakeBindingTarget("ArrowDown"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::SQUARE, CInputProviderEmscripten::MakeBindingTarget("KeyA"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::CROSS, CInputProviderEmscripten::MakeBindingTarget("KeyZ"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::TRIANGLE, CInputProviderEmscripten::MakeBindingTarget("KeyS"));
		bindingManager.SetSimpleBinding(0, PS2::CControllerInfo::CIRCLE, CInputProviderEmscripten::MakeBindingTarget("KeyX"));
	}

	EMSCRIPTEN_RESULT result = EMSCRIPTEN_RESULT_SUCCESS;

	result = emscripten_set_keydown_callback("#outputCanvas", nullptr, false, &keyboardCallback);
	assert(result == EMSCRIPTEN_RESULT_SUCCESS);

	result = emscripten_set_keyup_callback("#outputCanvas", nullptr, false, &keyboardCallback);
	assert(result == EMSCRIPTEN_RESULT_SUCCESS);

	//g_virtualMachine->Reset();
	printf("Reset VM\r\n");
}

void loadElf(std::string path)
{
	printf("Loading '%s'...\r\n", path.c_str());
	try
	{
		g_virtualMachine->Reset();
		g_virtualMachine->m_ee->m_os->BootFromFile(path);
	}
	catch(const std::exception& ex)
	{
		printf("Failed to start: %s.\r\n", ex.what());
		return;
	}
	printf("Starting...\r\n");
	g_virtualMachine->Resume();
}

void bootDiscImage(std::string path)
{
	printf("Loading '%s'...\r\n", path.c_str());
	try
	{
		CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, path);
		g_virtualMachine->Reset();
		g_virtualMachine->m_ee->m_os->BootFromCDROM();
	}
	catch(const std::exception& ex)
	{
		printf("Failed to start: %s.\r\n", ex.what());
		return;
	}
	printf("Starting...\r\n");
	g_virtualMachine->Resume();
}

EMSCRIPTEN_BINDINGS(Play)
{
	using namespace emscripten;

	function("loadElf", &loadElf);
	function("bootDiscImage", &bootDiscImage);
}
