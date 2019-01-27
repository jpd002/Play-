#include <cassert>
#include "android/JavaVM.h"
#include "com_virtualapplications_play_Bootable.h"

using namespace com::virtualapplications::play;

void Bootable_ClassInfo::PrepareClassInfo()
{
	auto env = Framework::CJavaVM::GetEnv();

	jclass tmpClazz = env->FindClass("com/virtualapplications/play/Bootable");
	Framework::CJavaVM::CheckException(env);
	assert(tmpClazz != NULL);
	clazz = reinterpret_cast<jclass>(env->NewGlobalRef(tmpClazz));
	assert(clazz != NULL);

	init = env->GetMethodID(clazz, "<init>", "()V");
	Framework::CJavaVM::CheckException(env);
	assert(init != NULL);

	path = env->GetFieldID(clazz, "path", "Ljava/lang/String;");
	Framework::CJavaVM::CheckException(env);
	assert(path != NULL);

	title = env->GetFieldID(clazz, "title", "Ljava/lang/String;");
	Framework::CJavaVM::CheckException(env);
	assert(title != NULL);

	coverUrl = env->GetFieldID(clazz, "coverUrl", "Ljava/lang/String;");
	Framework::CJavaVM::CheckException(env);
	assert(coverUrl != NULL);

	discId = env->GetFieldID(clazz, "discId", "Ljava/lang/String;");
	Framework::CJavaVM::CheckException(env);
	assert(discId != NULL);

	overview = env->GetFieldID(clazz, "overview", "Ljava/lang/String;");
	Framework::CJavaVM::CheckException(env);
	assert(overview != NULL);
}
