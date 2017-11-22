#include "../ui_shared/BootablesProcesses.h"
#include "../ui_shared/BootablesDbClient.h"
#include "com_virtualapplications_play_Bootable.h"
#include "NativeShared.h"

//Operation
//- Scan a first time

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_BootablesInterop_scanDirectory(JNIEnv* env, jobject obj, jstring scanPathString)
{
	try
	{
		auto scanPath = GetStringFromJstring(env, scanPathString);
		Log_Print("Scanning for bootables in '%s'\r\n", scanPath.c_str());
		ScanBootables(scanPath);
		ExtractDiscIds();
		FetchGameTitles();
		//FetchCoverUrls();
	}
	catch(const std::exception& exception)
	{
		Log_Print("Caught an exception: '%s'\r\n", exception.what());
	}
}

extern "C" JNIEXPORT jobjectArray Java_com_virtualapplications_play_BootablesInterop_getBootables(JNIEnv* env, jobject obj)
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	const auto& bootableClassInfo = com::virtualapplications::play::Bootable_ClassInfo::GetInstance();
	
	auto bootablesJ = env->NewObjectArray(bootables.size(), bootableClassInfo.clazz, NULL);
	
	for(unsigned int i = 0; i < bootables.size(); i++)
	{
		const auto& bootable = bootables[i];
		auto bootableJ = env->NewObject(bootableClassInfo.clazz, bootableClassInfo.init);
		
		jstring pathString = env->NewStringUTF(bootable.path.string().c_str());
		env->SetObjectField(bootableJ, bootableClassInfo.path, pathString);
		
		jstring titleString = env->NewStringUTF(bootable.title.c_str());
		env->SetObjectField(bootableJ, bootableClassInfo.title, titleString);
		
		env->SetObjectArrayElement(bootablesJ, i, bootableJ);
	}
	
	return bootablesJ;
}

extern "C" JNIEXPORT void Java_com_virtualapplications_play_BootablesInterop_setLastBootedTime(JNIEnv* env, jobject obj, jstring bootablePathString, jlong lastBootedTime)
{
	auto bootablePath = boost::filesystem::path(GetStringFromJstring(env, bootablePathString));
	BootablesDb::CClient::GetInstance().SetLastBootedTime(bootablePath, lastBootedTime);
}
