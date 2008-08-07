#ifndef _ACCELERATORTABLE_H_
#define _ACCELERATORTABLE_H_

#include "win32/Window.h"

namespace Framework
{
	namespace Win32
	{
		class CAcceleratorTable
		{
		public:
						CAcceleratorTable(HACCEL);
			virtual		~CAcceleratorTable();

						operator HACCEL();
		private:
			HACCEL		m_accel;
		};
	}
}

#endif
