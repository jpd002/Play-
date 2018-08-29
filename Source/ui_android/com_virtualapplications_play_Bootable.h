#include <jni.h>
#include "Singleton.h"

namespace com
{
	namespace virtualapplications
	{
		namespace play
		{
			class Bootable_ClassInfo : public CSingleton<Bootable_ClassInfo>
			{
			public:
				void PrepareClassInfo();

				jclass clazz = NULL;
				jmethodID init = NULL;
				jfieldID path = NULL;
				jfieldID discId = NULL;
				jfieldID title = NULL;
				jfieldID coverUrl = NULL;
			};
		}
	}
}
