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

	echo "<style>table{border-collapse: collapse;}td,th{border: 1px solid #dddddd;text-align:left;padding:8px;white-space:nowrap;}tr:nth-child(even){background-color: #dddddd;}</style>" > Changelog.html
	echo "<table> <tr> <th> Date </th> <th> Author </th> <th> Commit Hash </th> <th> Description </th> </tr>" >> Changelog.html
	git log --date="format:%%Y-%%m-%%d" --pretty="format:<tr> <td> %%ad </td> <td> %%an </td> <td><a href=\"https://github.com/jpd002/Play-/commit/%%H\">%%h</a></td> <td>%%s</td> </tr>" >> Changelog.html
	echo "</table>" >> Changelog.html

	"C:\Program Files (x86)\NSIS\makensis.exe" ./installer_win32/%INSTALLER_SCRIPT%
	
	mkdir %REPO_COMMIT_SHORT%
	move installer_win32\*.exe %REPO_COMMIT_SHORT%
)

if "%BUILD_PSFPLAYER%" == "ON" (
	cmake .. -G"%BUILD_TYPE%" -T v141_xp -DBUILD_PLAY=off -DBUILD_TESTS=off -DBUILD_PSFPLAYER=on
	if !errorlevel! neq 0 exit /b !errorlevel!
	
	cmake --build . --config %CONFIG_TYPE%
	if !errorlevel! neq 0 exit /b !errorlevel!
	
	cd ..
	"C:\Program Files (x86)\NSIS\makensis.exe" ./tools/PsfPlayer/installer_win32/%INSTALLER_SCRIPT%

	mkdir %REPO_COMMIT_SHORT%
	move tools\PsfPlayer\installer_win32\*.exe %REPO_COMMIT_SHORT%
)

if "%BUILD_PSFAOT%" == "ON" (
	cmake .. -G"%BUILD_TYPE%" -T v141_xp -DBUILD_PLAY=off -DBUILD_TESTS=off -DBUILD_PSFPLAYER=on -DBUILD_AOT_CACHE=on
	if !errorlevel! neq 0 exit /b !errorlevel!
	
	cmake --build . --config %CONFIG_TYPE%
	if !errorlevel! neq 0 exit /b !errorlevel!
)
