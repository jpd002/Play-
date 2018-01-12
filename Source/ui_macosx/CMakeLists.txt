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
			${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/build_cmake/zlib-1.2.8
			${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.8
		)
	endif()
endif()
list(APPEND PROJECT_LIBS ZLIB::ZLIB)

find_package(Threads REQUIRED)
if(CMAKE_THREAD_LIBS_INIT)
	list(APPEND PROJECT_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()

set(OSX_SOURCES
	ApplicationDelegate.mm
	AudioSettingsViewController.mm
	Globals.cpp
	GSH_OpenGLMacOSX.cpp
	main.mm
	OutputWindowController.mm
	OutputWindow.mm
	PH_HidMacOSX.cpp
	PreferencesWindowController.mm
	VfsManagerBindings.mm
	VfsManagerViewController.mm
	VideoSettingsViewController.mm
)

set(OSX_HEADERS
	Purei_Prefix.pch
	ApplicationDelegate.h
	AudioSettingsViewController.h
	Globals.h
	GSH_OpenGLMacOSX.h
	OutputWindowController.h
	OutputWindow.h
	PH_HidMacOSX.h
	PreferenceDefs.h
	PreferencesWindowController.h
	VfsManagerBindings.h
	VfsManagerViewController.h
	VideoSettingsViewController.h
)

# these are the OS X Interface Builder Files
set(OSX_XIBS
	English.lproj/PreferencesWindow
	English.lproj/MainMenu
	English.lproj/VideoSettingsView
	English.lproj/OutputWindow
	English.lproj/AudioSettingsView
	English.lproj/VfsManagerView
)

set(OSX_RES
	${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.icns
	${CMAKE_CURRENT_SOURCE_DIR}/../../patches.xml
)
# Add our Executable
add_executable(Play MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS} ${OSX_RES})
set_target_properties(Play PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

# Probably a better way to set the framework link libraries.
target_link_libraries(Play ${PROJECT_LIBS} "-ObjC -framework Cocoa -framework OpenGL -framework OpenAL -framework IOKit -framework AppKit -framework CoreData -framework Foundation")
target_include_directories(Play PRIVATE
	./
)
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

# Make sure we can find the 'ibtool' program. If we can NOT find it we
# skip generation of this project
find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
if(${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
	message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
endif()

# Compile the .xib files using the 'ibtool' program with the destination being the app package
foreach(xib ${OSX_XIBS})
	add_custom_command(TARGET Play POST_BUILD 
			COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
				--compile ${CMAKE_CFG_INTDIR}/Play.app/Contents/Resources/${xib}.nib
				${CMAKE_CURRENT_SOURCE_DIR}/${xib}.xib
			COMMENT "${CMAKE_CURRENT_SOURCE_DIR}/${xib}.xib")

endforeach()
