#ifndef _STDAFX_H_
#define _STDAFX_H_

#define _VARIADIC_MAX 8
#define NOMINMAX

#include <windows.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/signals2.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "Types.h"
#include "Stream.h"
#include "StdStream.h"
#include "xml/Node.h"
#include "make_unique.h"

#include "win32/Window.h"
#include "win32/Button.h"
#include "win32/Edit.h"
#include "win32/ListBox.h"
#include "win32/ListView.h"

#endif
