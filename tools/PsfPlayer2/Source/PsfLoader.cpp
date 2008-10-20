#include "PsfLoader.h"
#include "StdStream.h"
#include "ps2/PsfDevice.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

using namespace Framework;
using namespace Psx;
using namespace boost;
using namespace std;

void CPsfLoader::LoadPsf(CPsxVm& virtualMachine, const char* pathString, CPsfBase::TagMap* tags)
{
	CStdStream input(pathString, "rb");
	CPsfBase psfFile(input);
	if(psfFile.GetVersion() != CPsfBase::VERSION_PLAYSTATION)
	{
		throw runtime_error("Not a PlayStation psf.");
	}
	const char* libPath = psfFile.GetTagValue("_lib");
	if(tags)
	{
		tags->insert(psfFile.GetTagsBegin(), psfFile.GetTagsEnd());
	}
	{
		uint32 sp = 0, pc = 0;
		if(libPath != NULL)
		{
			filesystem::path path(pathString); 
			path = path.branch_path() / libPath;
			LoadPsf(virtualMachine, path.string().c_str());
			sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
			pc = virtualMachine.GetCpu().m_State.nPC;
		}
		virtualMachine.LoadExe(psfFile.GetProgram());
		if(sp != 0)
		{
			virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
		}
		if(pc != 0)
		{
			virtualMachine.GetCpu().m_State.nPC = pc;
		}
	}
	{
		unsigned int currentLib = 2;
		while(1)
		{
			string libName = "_lib" + lexical_cast<string>(currentLib);
			const char* libPath = psfFile.GetTagValue(libName.c_str());
			if(libPath == NULL)
			{
				break;
			}
			filesystem::path path(pathString); 
			path = path.branch_path() / libPath;
			uint32 sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
			uint32 pc = virtualMachine.GetCpu().m_State.nPC;
			LoadPsf(virtualMachine, path.string().c_str());
			virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
			virtualMachine.GetCpu().m_State.nPC = pc;
			currentLib++;
		}
	}
}

void CPsfLoader::LoadPsf2(PS2::CPsfVm& virtualMachine, const char* pathString, CPsfBase::TagMap* tags)
{
	CStdStream input(pathString, "rb");
    PS2::CPsfDevice::PsfFile psfFile(new CPsfBase(input));
	if(psfFile->GetVersion() != CPsfBase::VERSION_PLAYSTATION2)
	{
		throw runtime_error("Not a PlayStation2 psf.");
	}
    virtualMachine.LoadPsf(psfFile);
}
