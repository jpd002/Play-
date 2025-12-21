#include "NamcoSys246Driver.h"
#include "string_format.h"
#include "AppConfig.h"
#include "PS2VM.h"
#include "PS2VM_Preferences.h"
#include "../ArcadeDefinition.h"

#include "StdStreamUtils.h"
#include "DiskUtils.h"
#include "hdd/HddDefs.h"
#include "hdd/ApaDefs.h"
#include "discimages/ChdImageStream.h"
#include "iop/ioman/McDumpDevice.h"
#include "iop/ioman/HardDiskDumpDevice.h"
#include "iop/namco_sys246/Iop_NamcoSys246.h"
#include "iop/namco_sys246/Iop_NamcoAcCdvd.h"
#include "iop/namco_sys246/Iop_NamcoAcAta.h"
#include "iop/namco_sys246/Iop_NamcoAcRam.h"
#include "iop/namco_sys246/Iop_NamcoPadMan.h"

void CNamcoSys246Driver::PrepareEnvironment(CPS2VM* virtualMachine, const ARCADE_MACHINE_DEF& def)
{
	fs::path arcadeRomPath = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_ARCADEROMS_DIRECTORY);
	auto baseId = def.parent.empty() ? def.id : def.parent;

	//Mount CDVD
	if(!def.cdvdFileName.empty())
	{
		fs::path cdvdPath = LocateImageFile(def, def.cdvdFileName);

		//Try to create the optical media for sanity checks (will throw exceptions on errors).
		DiskUtils::CreateOpticalMediaFromPath(cdvdPath);

		CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, cdvdPath);

		virtualMachine->CDROM0_SyncPath();
	}

	//Mount HDD
	if(!def.hddFileName.empty())
	{
		fs::path hddPath = LocateImageFile(def, def.hddFileName);

		auto imageStream = std::make_unique<CChdImageStream>(std::make_unique<Framework::CStdStream>(hddPath.string().c_str(), "rb"));
		if(imageStream->GetUnitSize() != Hdd::g_sectorSize)
		{
			throw std::runtime_error(string_format("Sector size mismatch in HDD image file ('%s'). Make sure the CHD file was created with the 'createhd' option.", def.hddFileName.c_str()));
		}

		auto iopBios = dynamic_cast<CIopBios*>(virtualMachine->m_iop->m_bios.get());

		Hdd::APA_HEADER apaHeader;
		imageStream->Read(&apaHeader, sizeof(Hdd::APA_HEADER));
		imageStream->Seek(0, Framework::STREAM_SEEK_SET);
		if(apaHeader.magic == Hdd::APA_HEADER_MAGIC)
		{
			auto device = std::make_shared<Iop::Ioman::CHardDiskDumpDevice>(std::move(imageStream));
			iopBios->GetIoman()->RegisterDevice("hdd0", device);
		}
		else
		{
			//This probably has a custom filesystem, so, register the AC ATA module for raw access
			auto acAtaModule = std::make_shared<Iop::Namco::CAcAta>(*iopBios, virtualMachine->m_iop->m_ram);
			acAtaModule->SetHddStream(std::move(imageStream));
			iopBios->RegisterModule(acAtaModule);
		}
	}

	//Override mc0 device with special device reading directly from zip file
	{
		auto mcDumpContents = ReadDongleData(def);
		auto device = std::make_shared<Iop::Ioman::CMcDumpDevice>(std::move(mcDumpContents));
		auto iopBios = dynamic_cast<CIopBios*>(virtualMachine->m_iop->m_bios.get());
		iopBios->GetIoman()->RegisterDevice("mc0", device);
		iopBios->GetIoman()->RegisterDevice("ac0", device);

		//Ridge Racer 5: Arcade Battle doesn't have any FILEIO in its IOPRP
		//Assuming that the BIOS image for arcade boards is version 2.0.5
		iopBios->SetDefaultImageVersion(2050);

		auto acRam = std::make_shared<Iop::Namco::CAcRam>(*iopBios, virtualMachine->m_iop->m_ram);
		iopBios->RegisterModule(acRam);
		iopBios->RegisterHleModuleReplacement("Arcade_Ext._Memory", acRam);

		auto acCdvdModule = std::make_shared<Iop::Namco::CAcCdvd>(*iopBios->GetSifman(), *iopBios->GetCdvdman(), virtualMachine->m_iop->m_ram, *acRam.get());
		acCdvdModule->SetOpticalMedia(virtualMachine->m_cdrom0.get());
		iopBios->RegisterModule(acCdvdModule);
		iopBios->RegisterHleModuleReplacement("ATA/ATAPI_driver", acCdvdModule);
		iopBios->RegisterHleModuleReplacement("CD/DVD_Compatible", acCdvdModule);

		//Taiko no Tatsujin loads and use these, but we don't have a proper HLE
		//for PADMAN at the version that's provided by the SYS2x6 BIOS.
		//Using our current HLE PADMAN causes the game to crash due to differences in structure layouts.
		//Bloody Roar 3 also loads PADMAN and expects some response from it.
		//Just provide a dummy module instead to make sure loading succeeds.
		//Games rely on JVS for input anyways, so, it shouldn't be a problem if they can't use PADMAN.
		auto padManModule = std::make_shared<Iop::Namco::CPadMan>();
		iopBios->RegisterHleModuleReplacement("rom0:PADMAN", padManModule);
		iopBios->RegisterHleModuleReplacement("rom0:SIO2MAN", padManModule);

		{
			auto namcoArcadeModule = std::make_shared<Iop::Namco::CSys246>(*iopBios->GetSifman(), *iopBios->GetSifcmd(), *acRam, def.id);
			iopBios->RegisterModule(namcoArcadeModule);
			iopBios->RegisterHleModuleReplacement("rom0:DAEMON", namcoArcadeModule);
			virtualMachine->m_pad->InsertListener(namcoArcadeModule.get());
			for(const auto& buttonDefPair : def.buttons)
			{
				const auto& buttonPair = buttonDefPair.second;
				assert(buttonPair.first == -1);
				namcoArcadeModule->SetButton(buttonDefPair.first, buttonPair.second);
			}
			switch(def.inputMode)
			{
			case ARCADE_MACHINE_DEF::INPUT_MODE::LIGHTGUN:
				virtualMachine->SetGunListener(namcoArcadeModule.get());
				namcoArcadeModule->SetJvsMode(Iop::Namco::CSys246::JVS_MODE::LIGHTGUN);
				namcoArcadeModule->SetScreenPosXform(def.screenPosXform);
				break;
			case ARCADE_MACHINE_DEF::INPUT_MODE::DRUM:
				namcoArcadeModule->SetJvsMode(Iop::Namco::CSys246::JVS_MODE::DRUM);
				break;
			case ARCADE_MACHINE_DEF::INPUT_MODE::DRIVE:
				namcoArcadeModule->SetJvsMode(Iop::Namco::CSys246::JVS_MODE::DRIVE);
				break;
			case ARCADE_MACHINE_DEF::INPUT_MODE::TOUCH:
				virtualMachine->SetTouchListener(namcoArcadeModule.get());
				namcoArcadeModule->SetJvsMode(Iop::Namco::CSys246::JVS_MODE::TOUCH);
				namcoArcadeModule->SetScreenPosXform(def.screenPosXform);
				break;
			default:
				break;
			}

			if(!def.boardId.empty())
			{
				namcoArcadeModule->SetBoardId(def.boardId);
			}
		}
	}

	virtualMachine->SetEeFrequencyScale(def.eeFreqScaleNumerator, def.eeFreqScaleDenominator);
	if((def.eeFreqScaleNumerator != 1) || (def.eeFreqScaleDenominator != 1))
	{
		//Adjust SPU sampling rate with EE frequency scale. Not quite sure this is right.
		uint32 baseSamplingRate = Iop::Spu2::CCore::DEFAULT_BASE_SAMPLING_RATE * def.eeFreqScaleNumerator / def.eeFreqScaleDenominator;
		virtualMachine->m_iop->m_spu2.GetCore(0)->SetBaseSamplingRate(baseSamplingRate);
		virtualMachine->m_iop->m_spu2.GetCore(1)->SetBaseSamplingRate(baseSamplingRate);
	}
}

void CNamcoSys246Driver::Launch(CPS2VM* virtualMachine, const ARCADE_MACHINE_DEF& def)
{
	//Boot mc0:/BOOT (from def)
	virtualMachine->m_ee->m_os->BootFromVirtualPath(def.boot.c_str(), {"DANGLE"});
}

std::vector<uint8> CNamcoSys246Driver::ReadDongleData(const ARCADE_MACHINE_DEF& def)
{
	struct DONGLE_SEARCH_INFO
	{
		std::string archiveFileName;
		std::string donglePath;
	};

	fs::path arcadeRomPath = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_ARCADEROMS_DIRECTORY);

	std::vector<DONGLE_SEARCH_INFO> dongleSearchInfos;
	dongleSearchInfos.push_back({def.id + ".zip", def.dongleFileName});
	if(!def.parent.empty())
	{
		dongleSearchInfos.push_back({def.parent + ".zip", string_format("%s/%s", def.id.c_str(), def.dongleFileName.c_str())});
	}

	for(const auto& dongleSearchInfo : dongleSearchInfos)
	{
		try
		{
			fs::path arcadeRomArchivePath = arcadeRomPath / dongleSearchInfo.archiveFileName;
			auto inputStream = Framework::CreateInputStdStream(arcadeRomArchivePath.native());
			Framework::CZipArchiveReader archiveReader(inputStream);
			auto header = archiveReader.GetFileHeader(dongleSearchInfo.donglePath.c_str());
			if(!header)
			{
				continue;
			}
			auto fileStream = archiveReader.BeginReadFile(dongleSearchInfo.donglePath.c_str());
			std::vector<uint8> dongleData;
			dongleData.resize(header->uncompressedSize);
			fileStream->Read(dongleData.data(), header->uncompressedSize);
			return dongleData;
		}
		catch(...)
		{
			//Something went wrong, continue with next search path...
		}
	}

	auto message = string_format("Failed to find file '%s' in archive(s):\n", def.dongleFileName.c_str());
	for(const auto& dongleSearchInfo : dongleSearchInfos)
	{
		message += string_format("- %s (%s)\n", dongleSearchInfo.archiveFileName.c_str(), dongleSearchInfo.donglePath.c_str());
	}
	throw std::runtime_error(message);
}

fs::path CNamcoSys246Driver::LocateImageFile(const ARCADE_MACHINE_DEF& def, const std::string& fileName)
{
	fs::path arcadeRomPath = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_ARCADEROMS_DIRECTORY);

	std::vector<std::string> imageSearchPaths;
	imageSearchPaths.push_back(def.id);
	if(!def.parent.empty())
	{
		imageSearchPaths.push_back(def.parent);
	}

	for(const auto& imageSearchPath : imageSearchPaths)
	{
		fs::path imagePath = arcadeRomPath / imageSearchPath / fileName;
		if(fs::exists(imagePath))
		{
			return imagePath;
		}
	}

	auto message = string_format("Failed to find file '%s' in location(s):\n", fileName.c_str());
	for(const auto& imageSearchPath : imageSearchPaths)
	{
		message += string_format("- %s\n", imageSearchPath.c_str());
	}
	throw std::runtime_error(message);
}
