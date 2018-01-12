cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayUI)

set(PROJECT_NAME "Play!")
set(PROJECT_Version 0.30)
add_definitions(-DPLAY_VERSION="${PROJECT_Version}")

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND PROJECT_LIBS PlayCore)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
	)
endif()
list(INSERT PROJECT_LIBS 0 gsh_opengl)

# If ICU is available, add its libraries because Framework might need its functions
find_package(ICUUC)
if(ICUUC_FOUND)
	list(APPEND PROJECT_LIBS ${ICUUC_LIBRARIES})
endif()

find_package(ZLIB)
if(NOT ZLIB_FOUND)
	MESSAGE("-- Using Provided zlib source")
	if(NOT TARGET ZLIB::ZLIB)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/zlib-1.2.8
			${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.8
		)
	endif()
endif()
list(APPEND PROJECT_LIBS ZLIB::ZLIB)

find_package(Threads REQUIRED)
if(CMAKE_THREAD_LIBS_INIT)
	list(APPEND PROJECT_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()

find_package(LIBEVDEV REQUIRED)
list(APPEND PROJECT_LIBS ${LIBEVDEV_LIBRARY})


set(QT_SOURCES
	main.cpp
	mainwindow.cpp
	ElidedLabel.cpp
	GSH_OpenGLQt.cpp
	StatsManager.cpp
	PH_HidUnix.cpp
	settingsdialog.cpp
	openglwindow.cpp
	memorycardmanagerdialog.cpp
	vfsmanagerdialog.cpp
	vfsmodel.cpp
	vfsdiscselectordialog.cpp
	VfsDevice.cpp
	controllerconfigdialog.cpp
	InputBindingManager.cpp
	bindingmodel.cpp
	inputeventselectiondialog.cpp
	GamePad/GamePadUtils.cpp
	GamePad/GamePadInputEventListener.cpp
	GamePad/GamePadDeviceListener.cpp
)

set(QT_MOC_HEADERS
	mainwindow.h
	ElidedLabel.h
	GSH_OpenGLQt.h
	StatsManager.h
	PH_HidUnix.h
	settingsdialog.h
	PreferenceDefs.h
	openglwindow.h
	memorycardmanagerdialog.h
	vfsmanagerdialog.h
	vfsmodel.h
	vfsdiscselectordialog.h
	VfsDevice.h
	controllerconfigdialog.h
	InputBindingManager.h
	bindingmodel.h
	inputeventselectiondialog.h
	GamePad/GamePadUtils.h
	GamePad/GamePadInputEventListener.h
	GamePad/GamePadDeviceListener.h
)

set(QT_UIS
	Qt_ui/mainwindow.ui
	Qt_ui/settingsdialog.ui
	Qt_ui/memorycardmanager.ui
	Qt_ui/vfsmanagerdialog.ui
	Qt_ui/vfsdiscselectordialog.ui
	Qt_ui/controllerconfigdialog.ui
	Qt_ui/inputeventselectiondialog.ui
)

set(QT_RESOURCES
	Qt_ui/resources.qrc
)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core)
list(APPEND PROJECT_LIBS Qt5::Widgets Qt5::Core)
add_definitions(${QT_DEFINITIONS})

QT5_ADD_RESOURCES(QT_RES_SOURCES ${QT_RESOURCES})
QT5_WRAP_UI(QT_UI_HEADERS ${QT_UIS})
QT5_WRAP_CPP(QT_MOC_SRCS ${QT_MOC_HEADERS})

add_executable(Play ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})
target_link_libraries(Play ${PROJECT_LIBS})
target_include_directories(Play PRIVATE
	./
	../../
	${LIBEVDEV_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)
