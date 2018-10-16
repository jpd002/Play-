cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayUI)

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

if(NOT TARGET Framework_OpenAl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkOpenAl
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkOpenAl
	)
endif()
list(APPEND PROJECT_LIBS Framework_OpenAl)
find_package(OpenAL REQUIRED)

if(NOT TARGET iCade_Static)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/iCade-iOS
		${CMAKE_CURRENT_BINARY_DIR}/iCade-iOS
	)
endif()
list(APPEND PROJECT_LIBS iCade_Static)

if(NOT TARGET SDWebImage_Static)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Dependencies/build_cmake/SDWebImage
		${CMAKE_CURRENT_BINARY_DIR}/SDWebImage
	)
endif()
list(APPEND PROJECT_LIBS SDWebImage_Static)

find_library(UIKIT_LIBRARY NAMES UIKit)
find_library(COREGRAPHICS_LIBRARY NAMES CoreGraphics)
find_library(GAMECONTROLLER_LIBRARY NAMES GameController)
find_library(QUARTZCORE_LIBRARY NAMES QuartzCore)
find_library(IMAGEIO_LIBRARY NAMES ImageIO)

set(OSX_SOURCES
	AppDelegate.mm
	BackgroundLayer.m
	CollectionView.m
	CoverViewCell.m
	CoverViewController.m
	EmulatorViewController.mm
	GlEsView.mm
	GSH_OpenGLiOS.cpp
	IosUtils.cpp
	main.mm
	SettingsViewController.mm
	SqliteDatabase.m
	VirtualPadButton.mm
	VirtualPadItem.mm
	VirtualPadStick.mm
	VirtualPadView.mm
)

set(OSX_HEADERS
	AppDelegate.h
	BackgroundLayer.h
	CollectionView.h
	CoverViewCell.h
	CoverViewController.h
	EmulatorViewController.h
	GlEsView.h
	GSH_OpenGLiOS.h
	IosUtils.h
	SettingsViewController.h
	SqliteDatabase.h
	VirtualPadButton.h
	VirtualPadItem.h
	VirtualPadStick.h
	VirtualPadView.h
)

set(OSX_RES
	${CMAKE_CURRENT_SOURCE_DIR}/../../games.db
	${CMAKE_CURRENT_SOURCE_DIR}/../../patches.xml
	${CMAKE_CURRENT_SOURCE_DIR}/Base.lproj/Main.storyboard
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon@2x.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/boxart.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/select.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/start.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/up.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/down.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/left.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/right.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/triangle.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/cross.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/circle.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/square.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/lr.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/analogstick.png
)

# Add our Executable
add_executable(Play MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS} ${OSX_RES})
set_target_properties(Play PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

# Probably a better way to set the framework link libraries.
target_link_libraries(Play ${UIKIT_LIBRARY})
target_link_libraries(Play ${COREGRAPHICS_LIBRARY})
target_link_libraries(Play ${GAMECONTROLLER_LIBRARY})
target_link_libraries(Play ${QUARTZCORE_LIBRARY})
target_link_libraries(Play ${IMAGEIO_LIBRARY})
target_link_libraries(Play ${PROJECT_LIBS} "-ObjC -lsqlite3 -framework OpenGLES -framework OpenAL")

# Set a custom plist file for the app bundle
# NOTE: for these values to be used Info.plist has to be edited
# NOTE: from cmake 3.7.0 you can use %b for month name abbreviations
string(TIMESTAMP DATE "%Y-%m-%d")
set_target_properties(
	Play
	PROPERTIES
		MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.virtualapplications.play"
		MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} Version ${PROJECT_Version}"
		MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_Version}"
		MACOSX_BUNDLE_BUNDLE_VERSION ${DATE}
		MACOSX_BUNDLE_COPYRIGHT "© Virtual Applications, 2017"
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
		XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
		RESOURCE "${OSX_RES}"
)
