#ifndef _STDAFX_H_
#define _STDAFX_H_

#define _VARIADIC_MAX 8
#define NOMINMAX
#define BOOST_OPTIONAL_USE_OLD_DEFINITION_OF_NONE

#include <windows.h>

#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>

#include "StdStream.h"
#include "Stream.h"
#include "Types.h"
#include "make_unique.h"
#include "xml/Node.h"

#include "win32/Button.h"
#include "win32/Edit.h"
#include "win32/ListBox.h"
#include "win32/ListView.h"
#include "win32/Window.h"

#endif
