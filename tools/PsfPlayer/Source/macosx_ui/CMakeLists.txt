
cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PsfPlayer)

if (NOT TARGET PsfCore)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/PsfCore
	)
endif()
list(APPEND PROJECT_LIBS PsfCore)

set (OSX_SOURCES
	ApplicationDelegate.mm
	PlaylistItem.mm
	Playlist.mm
	main.mm
)

set (OSX_HEADERS
	ApplicationDelegate.h
	PlaylistItem.h
	Playlist.h
	PsfPlayer_Prefix.pch
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
target_include_directories(PsfPlayer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
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
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
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
	COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/../../build_macosx/PsfPlayer.xcodeproj/TemplateIcon.icns ${CMAKE_CFG_INTDIR}/PsfPlayer.app/Contents/Resources/AppIcon.icns
)

# Compile the .xib files using the 'ibtool' program with the destination being the app package
foreach(xib ${OSX_XIBS})
	add_custom_command (TARGET PsfPlayer POST_BUILD 
			COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
				--compile ${CMAKE_CFG_INTDIR}/PsfPlayer.app/Contents/Resources/${xib}.nib
				${CMAKE_CURRENT_SOURCE_DIR}/${xib}.xib
			COMMENT "${CMAKE_CURRENT_SOURCE_DIR}/${xib}.xib")

endforeach()