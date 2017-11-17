cmake_minimum_required(VERSION 2.8)

option(TARGET_IOS "Enable building for iOS" OFF)

# macOS deployment target needs to be set before 'project' to work
if(APPLE AND NOT TARGET_IOS)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
endif()

if(ANDROID)
	message("-- Generating for Android --")
	set(TARGET_PLATFORM_ANDROID TRUE)
elseif(APPLE AND TARGET_IOS)
	message("-- Generating for iOS --")
	set(TARGET_PLATFORM_IOS TRUE)
elseif(APPLE)
	message("-- Generating for macOS --")
	set(TARGET_PLATFORM_MACOS TRUE)
elseif(WIN32)
	message("-- Generating for Win32 --")
	string(FIND ${CMAKE_GENERATOR} "Win64" HASWIN64)
	if(NOT HASWIN64 EQUAL -1)
		message("-- Arch: x64 --")
		set(TARGET_PLATFORM_WIN32_X64 TRUE)
	else()
		message("-- Arch: x86 --")
		set(TARGET_PLATFORM_WIN32_X86 TRUE)
	endif()
	set(TARGET_PLATFORM_WIN32 TRUE)
else()
	message("-- Generating for Unix compatible platform --")
	set(TARGET_PLATFORM_UNIX TRUE)
endif()

project(PsfPlayer)
set(PROJECT_LIBS)
set(PSF_PROJECT_NAME "PsfPlayer")
set(PSF_PROJECT_VERSION "1.05")

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

# flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
	set(CMAKE_CXX_FLAGS "/EHsc")
endif()

# definitions
if(NOT MSVC)
	MESSAGE("-- Build type: ${CMAKE_BUILD_TYPE}")
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_definitions(-DNDEBUG)
	else()
		add_definitions(-D_DEBUG)
	endif()
endif()

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_LIB)
	add_definitions(-D_UNICODE -DUNICODE)
endif()

if (NOT TARGET PsfCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/PsfCore
		${CMAKE_CURRENT_BINARY_DIR}/PsfCore
	)
endif()
list(APPEND PROJECT_LIBS PsfCore)

#UI
if(TARGET_PLATFORM_MACOS)

	set (OSX_SOURCES
		../Source/macosx_ui/ApplicationDelegate.mm
		../Source/macosx_ui/PlaylistItem.mm
		../Source/macosx_ui/Playlist.mm
		../Source/macosx_ui/main.mm
	)

	set (OSX_HEADERS
		../Source/macosx_ui/ApplicationDelegate.h
		../Source/macosx_ui/PlaylistItem.h
		../Source/macosx_ui/Playlist.h
		../Source/macosx_ui/PsfPlayer_Prefix.pch
	)

	# these are the OS X Interface Builder Files
	set (OSX_XIBS
		MainMenu
	)

	# Add our Executable
	add_executable(PsfPlayer MACOSX_BUNDLE ${OSX_SOURCES} ${OSX_HEADERS})
	set_target_properties(PsfPlayer PROPERTIES COMPILE_FLAGS "-x objective-c++")

	# Probably a better way to set the framework link libraries.
	target_link_libraries(PsfPlayer ${PROJECT_LIBS} "-ObjC -framework Cocoa -framework OpenGL -framework OpenAL -framework IOKit  -framework AppKit  -framework CoreData  -framework Foundation")
	target_include_directories(PsfPlayer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Source/macosx_ui/)
	# Set a custom plist file for the app bundle
	# NOTE: for these values to be used Info.plist has to be edited
	set_target_properties(
		PsfPlayer
		PROPERTIES
			MACOSX_BUNDLE_INFO_STRING "${PSF_PROJECT_NAME}"
			MACOSX_BUNDLE_GUI_IDENTIFIER "com.virtualapplications.PsfPlayer"
			MACOSX_BUNDLE_LONG_VERSION_STRING "${PSF_PROJECT_NAME} Version ${PSF_PROJECT_VERSION}"
			MACOSX_BUNDLE_BUNDLE_NAME ${PSF_PROJECT_NAME}
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PSF_PROJECT_VERSION}"
			MACOSX_BUNDLE_BUNDLE_VERSION ${PSF_PROJECT_NAME}
			MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2006-2017, Jean-Philip Desjardins. All Rights Reserved."
			MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
			MACOSX_BUNDLE_NSMAIN_NIB_FILE "MainMenu"
			MACOSX_BUNDLE_NSPRINCIPAL_CLASS "NSApplication"
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../Source/macosx_ui/Info.plist.in"
)

	# Make sure we can find the 'ibtool' program. If we can NOT find it we
	# skip generation of this project
	find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
		message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	endif()

	# Make sure the 'Resources' Directory is correctly created before we build
	add_custom_command (TARGET PsfPlayer PRE_BUILD COMMAND mkdir -p ${CMAKE_CFG_INTDIR}/PsfPlayer.app/Contents/Resources)

	# Copy Icon and patches
	add_custom_command (TARGET PsfPlayer POST_BUILD 
		COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/../build_macosx/PsfPlayer.xcodeproj/TemplateIcon.icns ${CMAKE_CFG_INTDIR}/PsfPlayer.app/Contents/Resources/AppIcon.icns
	)

	# Compile the .xib files using the 'ibtool' program with the destination being the app package
	foreach(xib ${OSX_XIBS})
		add_custom_command (TARGET PsfPlayer POST_BUILD 
				COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
					--compile ${CMAKE_CFG_INTDIR}/PsfPlayer.app/Contents/Resources/${xib}.nib
					${CMAKE_CURRENT_SOURCE_DIR}/../Source/macosx_ui/${xib}.xib
				COMMENT "${CMAKE_CURRENT_SOURCE_DIR}/../Source/macosx_ui/${xib}.xib")

	endforeach()
endif(TARGET_PLATFORM_MACOS)

