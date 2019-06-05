@echo off
setlocal enabledelayedexpansion

mkdir build
cd build

if "%BUILD_PLAY%" == "ON" (
	cmake .. -G"%BUILD_TYPE%" -T v141_xp -DUSE_QT=on -DCMAKE_PREFIX_PATH="C:\Qt\5.12\%QT_FLAVOR%"
	if !errorlevel! neq 0 exit /b !errorlevel!
	
	cmake --build . --config %CONFIG_TYPE%
	if !errorlevel! neq 0 exit /b !errorlevel!

	c:\Qt\5.12\%QT_FLAVOR%\bin\windeployqt.exe ./Source/ui_qt/Release --no-system-d3d-compiler --no-quick-import --no-opengl-sw --no-compiler-runtime --no-translations	
	
	cd ..
	"C:\Program Files (x86)\NSIS\makensis.exe" ./installer_win32/%INSTALLER_SCRIPT%
	
	cd ..
	cd ..
	mkdir %REPO_COMMIT_SHORT%
	move Play-Build\Play\installer_win32\*.exe %REPO_COMMIT_SHORT%
)

if "%BUILD_PSFPLAYER%" == "ON" (
	cmake .. -G"%BUILD_TYPE%" -T v141_xp -DBUILD_PLAY=off -DBUILD_TESTS=off -DBUILD_PSFPLAYER=on
	if !errorlevel! neq 0 exit /b !errorlevel!
	
	cmake --build . --config %CONFIG_TYPE%
	if !errorlevel! neq 0 exit /b !errorlevel!
	
	cd ..
	"C:\Program Files (x86)\NSIS\makensis.exe" ./tools/PsfPlayer/installer_win32/%INSTALLER_SCRIPT%

	cd ..
	cd ..
	mkdir %REPO_COMMIT_SHORT%
	move Play-Build\Play\tools\PsfPlayer\installer_win32\*.exe %REPO_COMMIT_SHORT%
)

if "%BUILD_PSFAOT%" == "ON" (
	cmake .. -G"%BUILD_TYPE%" -T v141_xp -DBUILD_PLAY=off -DBUILD_TESTS=off -DBUILD_PSFPLAYER=on -DBUILD_AOT_CACHE=on
	if !errorlevel! neq 0 exit /b !errorlevel!
	
	cmake --build . --config %CONFIG_TYPE%
	if !errorlevel! neq 0 exit /b !errorlevel!
)
