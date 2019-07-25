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

if(TARGET_PLATFORM_UNIX)
	find_package(LIBEVDEV REQUIRED)
	list(APPEND PROJECT_LIBS ${LIBEVDEV_LIBRARY})
	list(APPEND DEFINITIONS_LIST HAS_LIBEVDEV=1)
endif()

if(TARGET_PLATFORM_MACOS OR TARGET_PLATFORM_UNIX)
	if(NOT TARGET sh_openal)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../tools/PsfPlayer/Source/SH_OpenAL
			${CMAKE_CURRENT_BINARY_DIR}/SH_OpenAL
		)
	endif()
	list(APPEND PROJECT_LIBS sh_openal)
endif()

if(NOT TARGET ui_shared)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../ui_shared
		${CMAKE_CURRENT_BINARY_DIR}/ui_shared
	)
	endif()
list(APPEND PROJECT_LIBS ui_shared)

set(QT_SOURCES
	bootablelistdialog.cpp
	BootableModel.cpp
	ControllerConfig/bindingmodel.cpp
	ControllerConfig/bindingmodel.h
	ControllerConfig/controllerconfigdialog.cpp
	ControllerConfig/controllerconfigdialog.h
	ControllerConfig/inputeventselectiondialog.cpp
	ControllerConfig/inputeventselectiondialog.h
	CoverUtils.cpp
	CoverUtils.h
	main.cpp
	mainwindow.cpp
	mainwindow.h
	ContinuationChecker.cpp
	ContinuationChecker.h
	ElidedLabel.cpp
	ElidedLabel.h
	GSH_OpenGLQt.cpp
	GSH_OpenGLQt.h
	InputProviderQtKey.cpp
	InputProviderQtKey.h
	S3FileBrowser.cpp
	S3FileBrowser.h
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
)
if(LIBEVDEV_FOUND)
	set(QT_SOURCES
		${QT_SOURCES}
		unix/GamePadDeviceListener.cpp
		unix/GamePadDeviceListener.h
		unix/GamePadInputEventListener.cpp
		unix/GamePadInputEventListener.h
		unix/GamePadUtils.cpp
		unix/GamePadUtils.h
		unix/InputProviderEvDev.cpp
		unix/InputProviderEvDev.h
	)
endif()
if(TARGET_PLATFORM_WIN32)
	if(NOT TARGET Framework_Win32)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/build_cmake/FrameworkWin32
			${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
		)
	endif()
	list(APPEND PROJECT_LIBS Framework_Win32)
	
	set(QT_SOURCES
		${QT_SOURCES}
		../../tools/PsfPlayer/Source/win32_ui/SH_WaveOut.cpp
	)
	list(APPEND PROJECT_LIBS winmm)
	
	if(TARGET_PLATFORM_WIN32_X86)
		set(MANIFEST win32/Play-x86.manifest)
	else()
		set(MANIFEST win32/Play-x64.manifest)
	endif()

	set(QT_SOURCES
		${QT_SOURCES}
		win32/Res.rc
		win32/DebugSupport/ElfViewRes.rc
		${MANIFEST}
	)
	
	list(APPEND PROJECT_LIBS dinput8)
	list(APPEND PROJECT_LIBS xinput)
	
	if(DEBUGGER_INCLUDED)
		if(NOT TARGET PlayDebugSupport)
			add_subdirectory(
				${CMAKE_CURRENT_SOURCE_DIR}/win32/DebugSupport
				${CMAKE_CURRENT_BINARY_DIR}/PlayDebugSupport
			)
		endif()
		list(APPEND PROJECT_LIBS PlayDebugSupport)
	endif()
	
endif()
set(QT_MOC_HEADERS
	bootablelistdialog.h
	BootableModel.h
	ControllerConfig/bindingmodel.h
	ControllerConfig/controllerconfigdialog.h
	ControllerConfig/inputeventselectiondialog.h
	mainwindow.h
	ContinuationChecker.h
	ElidedLabel.h
	settingsdialog.h
	openglwindow.h
	memorycardmanagerdialog.h
	S3FileBrowser.h
	vfsmanagerdialog.h
	vfsmodel.h
	vfsdiscselectordialog.h
)

set(QT_UIS
	Qt_ui/bootablelistdialog.ui
	Qt_ui/controllerconfigdialog.ui
	Qt_ui/debugmenu.ui
	Qt_ui/inputeventselectiondialog.ui
	Qt_ui/mainwindow.ui
	Qt_ui/memorycardmanager.ui
	Qt_ui/s3filebrowser.ui
	Qt_ui/settingsdialog.ui
	Qt_ui/vfsmanagerdialog.ui
	Qt_ui/vfsdiscselectordialog.ui
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
	set(QT_SOURCES
		${QT_SOURCES}
		macos/InputProviderMacOsHid.cpp
		macos/InputProviderMacOsHid.h
	)
	set(OSX_RES
		${CMAKE_CURRENT_SOURCE_DIR}/macos/AppIcon.icns
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
			MACOSX_BUNDLE_COPYRIGHT "© Virtual Applications, 2019"
			MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
		RESOURCE "${OSX_RES}"
	)
	list(APPEND PROJECT_LIBS "-framework IOKit -framework CoreFoundation")
elseif(TARGET_PLATFORM_WIN32)
	set(QT_SOURCES
		${QT_SOURCES}
		win32/InputProviderDirectInput.cpp
		win32/InputProviderDirectInput.h
		win32/InputProviderXInput.cpp
		win32/InputProviderXInput.h
	)
	add_executable(Play WIN32 ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})
elseif(TARGET_PLATFORM_UNIX)
	add_executable(Play ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS})

	install(TARGETS Play DESTINATION bin RENAME Play PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../installer_unix/Play.desktop DESTINATION share/applications)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../icons/icon_base.png DESTINATION share/icons RENAME Play.png)
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
