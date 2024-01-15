cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayUI)

add_definitions(-DPLAY_VERSION="${PROJECT_Version}")

set(USE_GSH_VULKAN OFF)
find_package(Vulkan)
if(Vulkan_FOUND)
	set(USE_GSH_VULKAN ON)
	message("Building with Vulkan support.")
else()
	message("Building without Vulkan support. If you want to enable Vulkan, please make sure VULKAN_SDK is set.")
endif()

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

if(USE_GSH_VULKAN)
	if(NOT TARGET gsh_vulkan)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/../gs/GSH_Vulkan
			${CMAKE_CURRENT_BINARY_DIR}/gs/GSH_Vulkan
		)
	endif()
	list(INSERT PROJECT_LIBS 0 gsh_vulkan)
	list(APPEND DEFINITIONS_LIST HAS_GSH_VULKAN=1)
	set(GSH_VULKAN_QT_SRC
		GSH_VulkanQt.cpp
		GSH_VulkanQt.h
		vulkanwindow.cpp
		vulkanwindow.h
	)
	set(GSH_VULKAN_QT_MOC
		vulkanwindow.h
	)
endif()

if(TARGET_PLATFORM_UNIX)
	find_package(LIBEVDEV REQUIRED)
	list(APPEND PROJECT_LIBS ${LIBEVDEV_LIBRARY})
	list(APPEND DEFINITIONS_LIST HAS_LIBEVDEV=1)

	list(APPEND PROJECT_LIBS "-static-libgcc")
	list(APPEND PROJECT_LIBS "-static-libstdc++")

	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		# most unix system seem to implicitly link to libinotify, but not FreeBSD
		list(APPEND PROJECT_LIBS "libinotify.so")
	endif()
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
	BootableModel.cpp
	BootableModel.h
	BootableModelProxy.cpp
	BootableModelProxy.h
	ControllerConfig/controllerconfigdialog.cpp
	ControllerConfig/controllerconfigdialog.h
	ControllerConfig/inputbindingmodel.cpp
	ControllerConfig/inputbindingmodel.h
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
	InputProviderQtMouse.cpp
	InputProviderQtMouse.h
	settingsdialog.cpp
	settingsdialog.h
	openglwindow.cpp
	openglwindow.h
	outputwindow.cpp
	outputwindow.h
	memorycardmanagerdialog.cpp
	memorycardmanagerdialog.h
	PreferenceDefs.h
	QBootablesView.cpp
	QBootablesView.h
	QStringUtils.cpp
	QStringUtils.h
	QtUtils.cpp
	QtUtils.h
	vfsmanagerdialog.cpp
	vfsmanagerdialog.h
	vfsmodel.cpp
	vfsmodel.h
	vfsdiscselectordialog.cpp
	vfsdiscselectordialog.h
	VfsDevice.cpp
	VfsDevice.h
	${GSH_VULKAN_QT_SRC}
)
if(ENABLE_AMAZON_S3)
	set(QT_SOURCES
		${QT_SOURCES}
		S3FileBrowser.cpp
		S3FileBrowser.h
	)
	set(AMAZON_S3_QT_MOC
		S3FileBrowser.h
	)
endif()
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
			${CMAKE_CURRENT_SOURCE_DIR}/../../deps/Framework/build_cmake/FrameworkWin32
			${CMAKE_CURRENT_BINARY_DIR}/Framework_Win32
		)
	endif()
	list(APPEND PROJECT_LIBS Framework_Win32)
	
	set(QT_SOURCES
		${QT_SOURCES}
		../../tools/PsfPlayer/Source/ui_win32/SH_WaveOut.cpp
	)
	list(APPEND PROJECT_LIBS winmm)
	
	if(TARGET_PLATFORM_WIN32_X86)
		set(MANIFEST win32/Play-x86.manifest)
	else()
		set(MANIFEST win32/Play-x64.manifest)
	endif()

	set(RESOURCE_FILE win32/Resources/Res.rc)
	set(CMAKE_RC_FLAGS "/d PLAY_VERSION=${PROJECT_Version}")

	set(QT_SOURCES
		${QT_SOURCES}
		${RESOURCE_FILE}
		${MANIFEST}
	)
	
	list(APPEND PROJECT_LIBS dinput8)
	list(APPEND PROJECT_LIBS xinput)
endif()
	
if(DEBUGGER_INCLUDED)
	if(NOT TARGET PlayDebugSupport)
		add_subdirectory(
			${CMAKE_CURRENT_SOURCE_DIR}/DebugSupport
			${CMAKE_CURRENT_BINARY_DIR}/PlayDebugSupport
		)
	endif()
	list(APPEND PROJECT_LIBS PlayDebugSupport)
endif()
	
set(QT_MOC_HEADERS
	BootableModel.h
	BootableModelProxy.h
	ControllerConfig/controllerconfigdialog.h
	ControllerConfig/inputbindingmodel.h
	ControllerConfig/inputeventselectiondialog.h
	mainwindow.h
	ContinuationChecker.h
	ElidedLabel.h
	settingsdialog.h
	openglwindow.h
	outputwindow.h
	QBootablesView.h
	memorycardmanagerdialog.h
	vfsmanagerdialog.h
	vfsmodel.h
	vfsdiscselectordialog.h
	${AMAZON_S3_QT_MOC}
	${GSH_VULKAN_QT_MOC}
)

set(QT_UIS
	Qt_ui/bootableview.ui
	Qt_ui/controllerconfigdialog.ui
	Qt_ui/debugdockmenu.ui
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

set(QT_COMPONENTS Widgets Core)
if(TARGET_PLATFORM_UNIX)
	list(APPEND QT_COMPONENTS X11Extras)
endif()
find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})

list(APPEND PROJECT_LIBS Qt5::Widgets Qt5::Core)
if(TARGET_PLATFORM_UNIX)
	list(APPEND PROJECT_LIBS Qt5::X11Extras)
endif()

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
	FILE(GLOB ARCADE_DEFS ${CMAKE_CURRENT_SOURCE_DIR}/../../arcadedefs/*.arcadedef)
	set(OSX_RES
		${CMAKE_CURRENT_SOURCE_DIR}/macos/AppIcon.icns
		${CMAKE_CURRENT_SOURCE_DIR}/../../patches.xml
	)
	if(DEBUGGER_INCLUDED)
		list(APPEND OSX_RES ${CMAKE_CURRENT_SOURCE_DIR}/../../ee_functions.xml)
	endif()
	if(USE_GSH_VULKAN)
		list(APPEND OSX_RES $ENV{VULKAN_SDK}/../MoltenVK/dylib/macOS/libMoltenVk.dylib)
	endif()
	add_executable(Play MACOSX_BUNDLE ${QT_SOURCES} ${QT_MOC_SRCS} ${QT_RES_SOURCES} ${QT_UI_HEADERS} ${OSX_RES} ${ARCADE_DEFS})
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
	SET_SOURCE_FILES_PROPERTIES(${ARCADE_DEFS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/arcadedefs")
	list(APPEND PROJECT_LIBS "-framework IOKit -framework CoreFoundation")
	if(USE_GSH_VULKAN)
		target_include_directories(Play PRIVATE $ENV{VULKAN_SDK}/../MoltenVK/include)
	endif()
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

	install(TARGETS Play DESTINATION ${CMAKE_INSTALL_BINDIR}
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
		RENAME Play
	)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../icons/icon_base_512x512.png
		DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps
		RENAME org.purei.Play.png
	)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../icons/icon.svg
		DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
		RENAME org.purei.Play.svg
	)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../installer_unix/org.purei.Play.desktop
		DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
	)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../installer_unix/org.purei.Play.metainfo.xml
		DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
	)
	FILE(GLOB ARCADE_DEFS ${CMAKE_CURRENT_SOURCE_DIR}/../../arcadedefs/*.arcadedef)
	install(FILES ${ARCADE_DEFS}
		DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/arcadedefs
	)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../states.db")
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../states.db
			DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/
		)
	else()
		message(WARNING "didn't find states.db")
	endif()
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
	find_program(WINDEPLOYQT_EXE windeployqt HINTS "${QT_BINARY_DIRECTORY}")
	add_custom_command(TARGET Play POST_BUILD
		COMMAND "${WINDEPLOYQT_EXE}" $<IF:$<CONFIG:Debug>,--debug,--release>
									 --no-translations
									 --no-compiler-runtime
									 "$<TARGET_FILE:Play>"
	)
endif()
