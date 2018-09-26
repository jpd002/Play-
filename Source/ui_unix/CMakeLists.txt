cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayUI)

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

if (NOT TARGET glew_s)
add_subdirectory(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/glew-2.0.0
	${CMAKE_CURRENT_BINARY_DIR}/glew-2.0.0
)
endif()
list(APPEND PROJECT_LIBS glew_s)

if(TARGET_PLATFORM_UNIX)
	find_package(LIBEVDEV REQUIRED)
	list(APPEND PROJECT_LIBS ${LIBEVDEV_LIBRARY})
	list(APPEND DEFINITIONS_LIST HAS_LIBEVDEV=1)
endif()

set(QT_SOURCES
	ControllerConfig/bindingmodel.cpp
	ControllerConfig/bindingmodel.h
	ControllerConfig/controllerconfigdialog.cpp
	ControllerConfig/controllerconfigdialog.h
	ControllerConfig/inputeventselectiondialog.cpp
	ControllerConfig/inputeventselectiondialog.h
	main.cpp
	mainwindow.cpp
	mainwindow.h
	ElidedLabel.cpp
	ElidedLabel.h
	GSH_OpenGLQt.cpp
	GSH_OpenGLQt.h
	StatsManager.cpp
	StatsManager.h
	PH_HidUnix.cpp
	PH_HidUnix.h
	settingsdialog.cpp
	settingsdialog.h
	openglwindow.cpp
	openglwindow.h
	memorycardmanagerdialog.cpp
	memorycardmanagerdialog.h
	PreferenceDefs.h
	QStringUtils.cpp
	QStringUtils.h
	vfsmanagerdialog.cpp
	vfsmanagerdialog.h
	vfsmodel.cpp
	vfsmodel.h
	vfsdiscselectordialog.cpp
	vfsdiscselectordialog.h
	VfsDevice.cpp
	VfsDevice.h
	InputBindingManager.cpp
	InputBindingManager.h
	GamePad/GamePadUtils.cpp
	GamePad/GamePadUtils.h
)
if(LIBEVDEV_FOUND)
	set(QT_SOURCES
		${QT_SOURCES}
		GamePad/GamePadInputEventListener.cpp
		GamePad/GamePadInputEventListener.h
		GamePad/GamePadDeviceListener.cpp
		GamePad/GamePadDeviceListener.h
	)
endif()
set(QT_MOC_HEADERS
	ControllerConfig/bindingmodel.h
	ControllerConfig/controllerconfigdialog.h
	ControllerConfig/inputeventselectiondialog.h
	mainwindow.h
	ElidedLabel.h
	settingsdialog.h
	openglwindow.h
	memorycardmanagerdialog.h
	vfsmanagerdialog.h
	vfsmodel.h
	vfsdiscselectordialog.h
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

if(TARGET_PLATFORM_MACOS)
	set(OSX_RES
		${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.icns
		${CMAKE_CURRENT_SOURCE_DIR}/../../patches.xml
	)
	add_executable(Play MACOSX_BUNDLE ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS} ${OSX_RES})
	# Set a custom plist file for the app bundle
	# NOTE: for these values to be used Info.plist has to be edited
	# NOTE: from cmake 3.7.0 you can use %b for month name abbreviations
	string(TIMESTAMP DATE "%d-%m-%Y")
	set_target_properties(
		Play
		PROPERTIES
			MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}"
			MACOSX_BUNDLE_GUI_IDENTIFIER "com.virtualapplications.Play"
			MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} Version ${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_Version}"
			MACOSX_BUNDLE_BUNDLE_VERSION ${DATE}
			MACOSX_BUNDLE_COPYRIGHT "© Virtual Applications, 2017"
			MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
		RESOURCE "${OSX_RES}"
	)
else()
	add_executable(Play ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})
endif()

target_link_libraries(Play ${PROJECT_LIBS})
target_compile_definitions(Play PRIVATE ${DEFINITIONS_LIST})

target_include_directories(Play PRIVATE
	./
	../../
	${LIBEVDEV_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

if(TARGET_PLATFORM_WIN32)
	add_custom_command(TARGET Play POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
		$<TARGET_FILE:Qt5::Core> $<TARGET_FILE_DIR:Play>
	)
	add_custom_command(TARGET Play POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
		$<TARGET_FILE:Qt5::Widgets> $<TARGET_FILE_DIR:Play>
	)
	add_custom_command(TARGET Play POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
		$<TARGET_FILE:Qt5::Gui> $<TARGET_FILE_DIR:Play>
	)
endif()
