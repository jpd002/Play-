set REPO_COMMIT_SHORT=%APPVEYOR_REPO_COMMIT:~0,8%
appveyor SetVariable -Name REPO_COMMIT_SHORT -Value %REPO_COMMIT_SHORT%
mkdir %REPO_COMMIT_SHORT%

cd ..
rename Play PlaySource
mkdir Play
cd Play

git clone -q https://github.com/jpd002/Play-Build.git Play-Build
cd Play-Build
git submodule update -q --init --recursive
git submodule foreach -q "git checkout -q master"
cd Dependencies
git submodule update --init

cd ..
rd /S /Q Play
move ..\..\PlaySource Play
cd Play

mkdir build
cd build

if "%BUILD_PLAY%" == "ON" (
	cmake .. -G"%BUILD_TYPE%" -T v141_xp -DUSE_QT=on -DCMAKE_PREFIX_PATH="C:\Qt\5.12\%QT_FLAVOR%"
	cmake --build . --config %CONFIG_TYPE%
	cd ..
	"C:\Program Files (x86)\NSIS\makensis.exe" ./installer_win32/%INSTALLER_SCRIPT%
	cd ..
	cd ..
	move Play-Build\Play\installer_win32\*.exe %REPO_COMMIT_SHORT%
)

if "%BUILD_PSFPLAYER%" == "ON" (
	cmake .. -G"%BUILD_TYPE%" -T v141_xp -BUILD_PLAY=off -DBUILD_PSFPLAYER=on
	cmake --build . --config %CONFIG_TYPE%
	cd ..
	"C:\Program Files (x86)\NSIS\makensis.exe" ./tools/PsfPlayer/installer_win32/%INSTALLER_SCRIPT%
	cd ..
	cd ..
	move Play-Build\Play\tools\PsfPlayer\installer_win32\*.exe %REPO_COMMIT_SHORT%
)
