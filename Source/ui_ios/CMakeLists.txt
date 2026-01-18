cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayUI LANGUAGES C Swift)

set(USE_GSH_VULKAN OFF)
find_package(Vulkan)
if(Vulkan_FOUND)
	set(USE_GSH_VULKAN ON)
	message("Building with Vulkan support.")
else()
	message("Building without Vulkan support. If you want to enable Vulkan, please make sure VULKAN_SDK is set and place its path inside CMAKE_PREFIX_PATH.")
endif()

if(NOT TARGET PlayCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/Source
	)
endif()
list(APPEND UI_IOS_PROJECT_LIBS PlayCore)

if(NOT TARGET gsh_opengl)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_OpenGL
		${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_OpenGL
	)
endif()
list(INSERT UI_IOS_PROJECT_LIBS 0 gsh_opengl)

if(USE_GSH_VULKAN)
	if(NOT TARGET gsh_vulkan)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_Vulkan
			${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_Vulkan
		)
	endif()
	list(INSERT UI_IOS_PROJECT_LIBS 0 gsh_vulkan)
	list(APPEND UI_IOS_DEFINITIONS_LIST HAS_GSH_VULKAN=1)
endif()

if(NOT TARGET sh_openal)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../sound/SH_OpenAL
		${CMAKE_CURRENT_BINARY_DIR}/SH_OpenAL
	)
endif()
list(APPEND UI_IOS_PROJECT_LIBS sh_openal)

if(NOT TARGET iCade_Static)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/build_cmake/iCade-iOS
		${CMAKE_CURRENT_BINARY_DIR}/iCade-iOS
	)
endif()
list(APPEND UI_IOS_PROJECT_LIBS iCade_Static)

if(NOT TARGET SDWebImage_Static)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/build_cmake/SDWebImage
		${CMAKE_CURRENT_BINARY_DIR}/SDWebImage
	)
endif()
list(APPEND UI_IOS_PROJECT_LIBS SDWebImage_Static)

if(NOT TARGET ui_shared)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../ui_shared
		${CMAKE_CURRENT_BINARY_DIR}/ui_shared
	)
endif()
list(APPEND UI_IOS_PROJECT_LIBS ui_shared)

if(NOT TARGET AltKit)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../../deps/AltKit
		${CMAKE_CURRENT_BINARY_DIR}/AltKit
	)
endif()
list(APPEND UI_IOS_PROJECT_LIBS AltKit)

find_library(UIKIT_LIBRARY NAMES UIKit)
find_library(COREGRAPHICS_LIBRARY NAMES CoreGraphics)
find_library(GAMECONTROLLER_LIBRARY NAMES GameController)
find_library(QUARTZCORE_LIBRARY NAMES QuartzCore)
find_library(IMAGEIO_LIBRARY NAMES ImageIO)

set(OSX_SOURCES
	AltServerJitService.mm
	StikDebugJitService.mm
	AppDelegate.mm
	JITInitializer.mm
	BackgroundLayer.m
	CoverViewCell.m
	CoverViewController.mm
	DebuggerSimulator.cpp
	EmulatorViewController.mm
	GSH_OpenGLiOS.cpp
	main.mm
	PreferenceDefs.h
	RenderView.mm
	SaveStateViewController.m
	SettingsListSelectorViewController.mm
	SettingsViewController.mm
	VirtualPadButton.mm
	VirtualPadItem.mm
	VirtualPadStick.mm
	VirtualPadView.mm
)

set(OSX_HEADERS
	AltServerJitService.h
	StikDebugJitService.h
	JITInitializer.h
	Bridging-Header.h
	AppDelegate.h
	BackgroundLayer.h
	CoverViewCell.h
	CoverViewController.h
	DebuggerSimulator.h
	EmulatorViewController.h
	GSH_OpenGLiOS.h
	RenderView.h
	SaveStateViewController.h
	SettingsListSelectorViewController.h
	SettingsViewController.h
	VirtualPadButton.h
	VirtualPadItem.h
	VirtualPadStick.h
	VirtualPadView.h
)

set(OSX_RES
	${CMAKE_CURRENT_SOURCE_DIR}/../../GameConfig.xml
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

if(USE_GSH_VULKAN)
	list(APPEND OSX_SOURCES GSH_VulkaniOS.cpp)
	list(APPEND OSX_HEADERS GSH_VulkaniOS.h)
endif()

# Add our Executable
add_executable(Play MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS} ${OSX_RES})
target_compile_options(Play PRIVATE $<$<NOT:$<COMPILE_LANGUAGE:Swift>>:-x objective-c++ -fobjc-arc>)

# Probably a better way to set the framework link libraries.
target_link_libraries(Play ${UIKIT_LIBRARY})
target_link_libraries(Play ${COREGRAPHICS_LIBRARY})
target_link_libraries(Play ${GAMECONTROLLER_LIBRARY})
target_link_libraries(Play ${QUARTZCORE_LIBRARY})
target_link_libraries(Play ${IMAGEIO_LIBRARY})
target_link_libraries(Play ${UI_IOS_PROJECT_LIBS} "-ObjC -lsqlite3 -framework OpenAL")
target_include_directories(Play PRIVATE
    "${CMAKE_SOURCE_DIR}/Source/ui_ios/BreakpointJIT/BreakpointJIT.framework/Headers"
)

# BreakpointJIT framework search path
target_link_options(Play PRIVATE "-F${CMAKE_CURRENT_SOURCE_DIR}/../../deps/BreakpointJIT")
target_compile_definitions(Play PRIVATE ${UI_IOS_DEFINITIONS_LIST})

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
		XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/deps/BreakpointJIT"
		)

if(USE_GSH_VULKAN)
	set_target_properties(
		Play
		PROPERTIES
			XCODE_EMBED_FRAMEWORKS "$ENV{VULKAN_SDK}/lib/MoltenVK.xcframework"
			XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY TRUE
	)
endif()
