#include "../ui_shared/BootablesProcesses.h"
#include "../ui_shared/BootablesDbClient.h"
#include "com_virtualapplications_play_Bootable.h"
#include "NativeShared.h"

void fullScan(JNIEnv* env, jobjectArray rootDirectories)
{
	auto rootDirectoryCount = env->GetArrayLength(rootDirectories);
	for(int i = 0; i < rootDirectoryCount; i++)
	{
		auto rootDirectoryString = static_cast<jstring>(env->GetObjectArrayElement(rootDirectories, i));
		auto rootDirectory = GetStringFromJstring(env, rootDirectoryString);
		ScanBootables(rootDirectory, true);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_BootablesInterop_scanBootables(JNIEnv* env, jobject obj, jobjectArray rootDirectories)
{
	try
	{
		auto activeDirectories = GetActiveBootableDirectories();
		if(activeDirectories.empty())
		{
			fullScan(env, rootDirectories);
		}
		else
		{
			for(const auto& activeDirectory : activeDirectories)
			{
				ScanBootables(activeDirectory, false);
			}
		}
	}
	catch(const std::exception& exception)
	{
		Log_Print("Caught an exception: '%s'\r\n", exception.what());
	}
	FetchGameTitles();
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_BootablesInterop_fullScanBootables(JNIEnv* env, jobject obj, jobjectArray rootDirectories)
{
	try
	{
		fullScan(env, rootDirectories);
	}
	catch(const std::exception& exception)
	{
		Log_Print("Caught an exception: '%s'\r\n", exception.what());
	}
	FetchGameTitles();
}

extern "C" JNIEXPORT jobjectArray Java_com_virtualapplications_play_BootablesInterop_getBootables(JNIEnv* env, jobject obj, jint sortedMethod)
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables(sortedMethod);
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

		jstring coverUrlString = env->NewStringUTF(bootable.coverUrl.c_str());
		env->SetObjectField(bootableJ, bootableClassInfo.coverUrl, coverUrlString);

		jstring discIdString = env->NewStringUTF(bootable.discId.c_str());
		env->SetObjectField(bootableJ, bootableClassInfo.discId, discIdString);

		jstring overviewString = env->NewStringUTF(bootable.overview.c_str());
		env->SetObjectField(bootableJ, bootableClassInfo.overview, overviewString);

		env->SetObjectArrayElement(bootablesJ, i, bootableJ);
	}

	return bootablesJ;
}

extern "C" JNIEXPORT void Java_com_virtualapplications_play_BootablesInterop_setLastBootedTime(JNIEnv* env, jobject obj, jstring bootablePathString, jlong lastBootedTime)
{
	auto bootablePath = boost::filesystem::path(GetStringFromJstring(env, bootablePathString));
	BootablesDb::CClient::GetInstance().SetLastBootedTime(bootablePath, lastBootedTime);
}

extern "C" JNIEXPORT void Java_com_virtualapplications_play_BootablesInterop_UnregisterBootable(JNIEnv* env, jobject obj, jstring bootablePathString)
{
	auto bootablePath = boost::filesystem::path(GetStringFromJstring(env, bootablePathString));
	BootablesDb::CClient::GetInstance().UnregisterBootable(bootablePath);
}

extern "C" JNIEXPORT void Java_com_virtualapplications_play_BootablesInterop_PurgeInexistingFiles(JNIEnv* env, jobject obj)
{
	PurgeInexistingFiles();
}
