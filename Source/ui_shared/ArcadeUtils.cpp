#include "ArcadeUtils.h"
#include <nlohmann/json.hpp>
#include "string_format.h"
#include "PathUtils.h"
#include "StdStreamUtils.h"
#include "PS2VM.h"
#include "PS2VM_Preferences.h"
#include "AppConfig.h"
#include "BootablesDbClient.h"
#include "BootablesProcesses.h"
#include "ArcadeDefinition.h"

#include "arcadedrivers/NamcoSys246Driver.h"
#include "arcadedrivers/NamcoSys147Driver.h"

// clang-format off
static const std::pair<const char*, ARCADE_MACHINE_DEF::DRIVER> g_driverValues[] =
{
	{ "sys246", ARCADE_MACHINE_DEF::DRIVER::NAMCO_SYSTEM_246 },
	{ "sys147", ARCADE_MACHINE_DEF::DRIVER::NAMCO_SYSTEM_147 },
};

static const std::pair<const char*, PS2::CControllerInfo::BUTTON> g_buttonValues[] =
{
	{ "dpad_up", PS2::CControllerInfo::DPAD_UP },
	{ "dpad_down", PS2::CControllerInfo::DPAD_DOWN },
	{ "dpad_left", PS2::CControllerInfo::DPAD_LEFT },
	{ "dpad_right", PS2::CControllerInfo::DPAD_RIGHT },
	{ "select", PS2::CControllerInfo::SELECT },
	{ "start", PS2::CControllerInfo::START },
	{ "square", PS2::CControllerInfo::SQUARE },
	{ "triangle", PS2::CControllerInfo::TRIANGLE },
	{ "cross", PS2::CControllerInfo::CROSS },
	{ "circle", PS2::CControllerInfo::CIRCLE },
	{ "l1", PS2::CControllerInfo::L1 },
	{ "l2", PS2::CControllerInfo::L2 },
	{ "l3", PS2::CControllerInfo::L3 },
	{ "r1", PS2::CControllerInfo::R1 },
	{ "r2", PS2::CControllerInfo::R2 },
	{ "r3", PS2::CControllerInfo::R3 }
};

static const std::pair<const char*, ARCADE_MACHINE_DEF::INPUT_MODE> g_inputModeValues[] =
{
	{ "default", ARCADE_MACHINE_DEF::INPUT_MODE::DEFAULT },
	{ "lightgun", ARCADE_MACHINE_DEF::INPUT_MODE::LIGHTGUN },
	{ "drum", ARCADE_MACHINE_DEF::INPUT_MODE::DRUM },
	{ "drive", ARCADE_MACHINE_DEF::INPUT_MODE::DRIVE },
	{ "touch", ARCADE_MACHINE_DEF::INPUT_MODE::TOUCH },
};

static const std::pair<const char*, ARCADE_MACHINE_DEF::IO_MODE> g_ioModeValues[] =
{
	{ "default", ARCADE_MACHINE_DEF::IO_MODE::DEFAULT },
	{ "sys147_ai", ARCADE_MACHINE_DEF::IO_MODE::SYS147_AI },
};
// clang-format on

template <typename ValueType>
ValueType ParseEnumValue(const char* valueName, const std::pair<const char*, ValueType>* beginIterator, const std::pair<const char*, ValueType>* endIterator)
{
	auto valueIterator = std::find_if(beginIterator, endIterator,
	                                  [&](const auto& valuePair) { return strcmp(valuePair.first, valueName) == 0; });
	if(valueIterator == endIterator)
	{
		throw std::runtime_error(string_format("Unknown enum name '%s'.", valueName));
	}
	return valueIterator->second;
}

uint32 ParseHexStringValue(const std::string& value)
{
	uint32 result = 0;
	int scanCount = sscanf(value.c_str(), "0x%x", &result);
	assert(scanCount == 1);
	return result;
}

void ApplyPatchesFromArcadeDefinition(CPS2VM* virtualMachine, const ARCADE_MACHINE_DEF& def)
{
	for(const auto& patch : def.patches)
	{
		assert(patch.address < PS2::EE_RAM_SIZE);
		*reinterpret_cast<uint32*>(virtualMachine->m_ee->m_ram + patch.address) = patch.value;
	}
}

ARCADE_MACHINE_DEF ReadArcadeMachineDefinition(const fs::path& arcadeDefPath)
{
	auto parseButtons =
	    [](const nlohmann::json& buttonsObject) {
		    auto buttonMap = buttonsObject.get<std::map<std::string, std::string>>();
		    decltype(ARCADE_MACHINE_DEF::buttons) buttons;
		    for(const auto& buttonPair : buttonMap)
		    {
			    char* endPtr = nullptr;
			    const char* buttonNumber = buttonPair.first.c_str();
			    const char* buttonSelector = buttonPair.second.c_str();
			    int number = strtol(buttonPair.first.c_str(), &endPtr, 10);
			    if(endPtr == buttonPair.first.c_str())
			    {
				    throw std::runtime_error(string_format("Failed to parse button number '%s'.", buttonNumber));
			    }
			    //Accepted formats for buttonSelector
			    //- ${buttonName}
			    //- ${padIdx}:${buttonName}
			    std::string buttonName = buttonSelector;
			    int padIdx = -1;
			    if(auto colonPos = buttonName.find(':'); colonPos != std::string::npos)
			    {
				    padIdx = strtol(buttonName.c_str(), &endPtr, 10);
				    assert(endPtr == (buttonName.c_str() + colonPos));
				    buttonName = buttonName.substr(colonPos + 1);
			    }
			    auto buttonId = ParseEnumValue(buttonName.c_str(), std::begin(g_buttonValues), std::end(g_buttonValues));
			    auto selector = ARCADE_MACHINE_DEF::ButtonSelector{padIdx, buttonId};
			    buttons[number] = selector;
		    }
		    return buttons;
	    };

	auto parsePatches =
	    [](const nlohmann::json& patchesArray) {
		    std::vector<ARCADE_MACHINE_DEF::PATCH> patches;
		    for(const auto& jsonPatch : patchesArray)
		    {
			    ARCADE_MACHINE_DEF::PATCH patch;
			    if(!jsonPatch.contains("address") || !jsonPatch.contains("value")) continue;
			    patch.address = ParseHexStringValue(jsonPatch["address"]);
			    patch.value = ParseHexStringValue(jsonPatch["value"]);
			    patches.emplace_back(std::move(patch));
		    }
		    return patches;
	    };

	auto defString =
	    [&arcadeDefPath]() {
		    try
		    {
			    auto defStream = Framework::CreateInputStdStream(arcadeDefPath.native());
			    return defStream.ReadString();
		    }
		    catch(...)
		    {
			    throw std::runtime_error(string_format("Failed to read arcade definition file located at '%s'.",
			                                           fs::absolute(arcadeDefPath).string().c_str()));
		    }
	    }();

	auto defJson = nlohmann::json::parse(defString);
	ARCADE_MACHINE_DEF def;
	def.id = defJson["id"];
	if(defJson.contains("parent"))
	{
		def.parent = defJson["parent"];
	}
	if(defJson.contains("driver"))
	{
		std::string driverName = defJson["driver"];
		def.driver = ParseEnumValue(driverName.c_str(), std::begin(g_driverValues), std::end(g_driverValues));
	}
	def.name = defJson["name"];
	if(defJson.contains("dongle"))
	{
		def.dongleFileName = defJson["dongle"]["name"];
	}
	if(defJson.contains("cdvd"))
	{
		def.cdvdFileName = defJson["cdvd"]["name"];
	}
	if(defJson.contains("hdd"))
	{
		def.hddFileName = defJson["hdd"]["name"];
	}
	if(defJson.contains("nand"))
	{
		def.nandFileName = defJson["nand"]["name"];
		if(defJson["nand"].contains("mounts"))
		{
			def.nandMounts = defJson["nand"]["mounts"].get<std::map<std::string, uint32>>();
		}
	}
	if(defJson.contains("buttons"))
	{
		def.buttons = parseButtons(defJson["buttons"]);
	}
	if(defJson.contains("inputMode"))
	{
		std::string inputModeString = defJson["inputMode"];
		def.inputMode = ParseEnumValue(inputModeString.c_str(), std::begin(g_inputModeValues), std::end(g_inputModeValues));
	}
	if(defJson.contains("ioMode"))
	{
		std::string ioModeString = defJson["ioMode"];
		def.ioMode = ParseEnumValue(ioModeString.c_str(), std::begin(g_ioModeValues), std::end(g_ioModeValues));
	}
	if(defJson.contains("boardId"))
	{
		def.boardId = defJson["boardId"];
	}
	if(defJson.contains("screenPosXform"))
	{
		auto screenPosXformArray = defJson["screenPosXform"];
		if(screenPosXformArray.is_array() && (screenPosXformArray.size() >= 4))
		{
			for(int i = 0; i < 4; i++)
			{
				def.screenPosXform[i] = screenPosXformArray[i];
			}
		}
	}
	if(defJson.contains("eeFrequencyScale"))
	{
		auto eeFreqScaleArray = defJson["eeFrequencyScale"];
		if(eeFreqScaleArray.is_array() && (eeFreqScaleArray.size() >= 2))
		{
			def.eeFreqScaleNumerator = eeFreqScaleArray[0];
			def.eeFreqScaleDenominator = eeFreqScaleArray[1];
		}
	}
	def.boot = defJson["boot"];
	if(defJson.contains("patches"))
	{
		def.patches = parsePatches(defJson["patches"]);
	}
	return def;
}

void ApplyParentDefValues(ARCADE_MACHINE_DEF& def, const ARCADE_MACHINE_DEF& parentDef)
{
	if(def.nandFileName.empty())
	{
		def.nandFileName = parentDef.nandFileName;
	}
	if(def.nandMounts.empty())
	{
		def.nandMounts = parentDef.nandMounts;
	}
	if(def.buttons.empty())
	{
		def.buttons = parentDef.buttons;
	}
}

void ArcadeUtils::RegisterArcadeMachines()
{
	auto arcadeDefsPath = Framework::PathUtils::GetAppResourcesPath() / "arcadedefs";
	if(!fs::exists(arcadeDefsPath))
	{
		return;
	}

	//This set is used to track inactive arcadedefs we might have in the database
	std::set<fs::path> inactiveArcadeDefs;

	//Collect all arcadedef files we have registered in the database
	{
		auto arcadeBootables = BootablesDb::CClient::GetInstance().GetBootables(BootablesDb::CClient::SORT_METHOD_ARCADE);
		for(const auto& bootable : arcadeBootables)
		{
			inactiveArcadeDefs.insert(bootable.path.filename());
		}
	}

	for(const auto& entry : fs::directory_iterator(arcadeDefsPath))
	{
		auto arcadeDefPath = entry.path();
		auto arcadeDefFilename = arcadeDefPath.filename();
		//Remove this arcadedef from the inactive set
		inactiveArcadeDefs.erase(arcadeDefFilename);
		try
		{
			auto def = ReadArcadeMachineDefinition(arcadeDefsPath / arcadeDefFilename);
			BootablesDb::CClient::GetInstance().RegisterBootable(arcadeDefFilename, "", "", BootableUtils::PS2_ARCADE);
			BootablesDb::CClient::GetInstance().SetTitle(arcadeDefFilename, def.name.c_str());
		}
		catch(const std::exception& exception)
		{
			printf("Warning: Failed to register arcade machine '%s': %s\r\n",
			       arcadeDefFilename.string().c_str(), exception.what());
		}
	}

	//Prune any remaining arcadedef
	for(const auto& inactiveArcadeDef : inactiveArcadeDefs)
	{
		BootablesDb::CClient::GetInstance().UnregisterBootable(inactiveArcadeDef);
	}
}

static CNamcoSys246Driver g_sys246Driver;
static CNamcoSys147Driver g_sys147Driver;
// clang-format off
static CArcadeDriver* g_drivers[] =
{
	nullptr,
	&g_sys246Driver,
	&g_sys147Driver,
};
// clang-format on

void ArcadeUtils::BootArcadeMachine(CPS2VM* virtualMachine, const fs::path& arcadeDefFilename)
{
	auto arcadeDefsPath = Framework::PathUtils::GetAppResourcesPath() / "arcadedefs";
	auto def = ReadArcadeMachineDefinition(arcadeDefsPath / arcadeDefFilename);
	if(!def.parent.empty())
	{
		auto parentDefPath = arcadeDefsPath / (def.parent + ".arcadedef");
		auto parentDef = ReadArcadeMachineDefinition(parentDefPath);
		ApplyParentDefValues(def, parentDef);
	}

	//Reset PS2VM
	virtualMachine->Pause();
	virtualMachine->Reset(PS2::EE_EXT_RAM_SIZE, PS2::IOP_EXT_RAM_SIZE);

	if(def.driver == ARCADE_MACHINE_DEF::DRIVER::UNKNOWN)
	{
		throw std::runtime_error("Arcade driver unspecified.");
	}

	auto driver = g_drivers[def.driver];
	driver->PrepareEnvironment(virtualMachine, def);
	driver->Launch(virtualMachine, def);

	ApplyPatchesFromArcadeDefinition(virtualMachine, def);

	virtualMachine->BeforeExecutableReloaded =
	    [def](CPS2VM* virtualMachine) {
		    auto driver = g_drivers[def.driver];
		    driver->PrepareEnvironment(virtualMachine, def);
	    };

	virtualMachine->AfterExecutableReloaded =
	    [def](CPS2VM* virtualMachine) {
		    ApplyPatchesFromArcadeDefinition(virtualMachine, def);
	    };

#ifndef DEBUGGER_INCLUDED
	virtualMachine->Resume();
#endif

	TryUpdateLastBootedTime(arcadeDefFilename);
}
