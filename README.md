# Play!

Play! is an attempt to create a PlayStation 2 emulator for Windows, macOS, UNIX, Android & iOS platforms.

Ongoing compatibility list can be found here: [Compatibility List Wiki](https://github.com/jpd002/Play-/wiki/Compatible-games).

For more information, please visit [purei.org](http://purei.org).

## Project Dependencies ##

### External Libraries ###
- [boost](http://boost.org)

### Repositories ###
- [Play! Dependencies](https://github.com/jpd002/Play-Dependencies)
- [Play! Framework](https://github.com/jpd002/Play--Framework) 
- [Play! CodeGen](https://github.com/jpd002/Play--CodeGen)
- [Nuanceur](https://github.com/jpd002/Nuanceur)

## Building ##

### General Setup ###

You can get almost everything needed to build the emulator by using the [Play! Build](https://github.com/jpd002/Play-Build) project. You can also checkout every repository individually if you wish to do so, but make sure your working copies share the same parent folder.

In the end, your setup should look like this:

C:\Projects
- CodeGen
- Dependencies
- Framework
- Nuanceur
- Play

### Common Building Instructions ###
First you'd need to clone Play-Build which provides you with the needed subprojects required to build Play!.
Then setup the submodules and the dependency submodule(s) too.
```
git clone https://github.com/jpd002/Play-.git
git submodule update -q --init --recursive
git submodule foreach "git checkout -q master"
cd Dependencies
git submodule update --init
cd ..
```
Currently macOS, Windows, Linux & Android builds support cmake build.

### Building for Windows ###
for Windows, you'd need to have cmake and DirectX sdk installed
```
cd Play/build_cmake
mkdir build
cd build
```
```
# Not specifying -G would automatically pick Visual Studio 32bit
cmake .. -G"Visual Studio 14 2015 Win64"
cmake --build . --config Release
```

### Building for macOS ###
if you dont have cmake installed, you can install it using brew with the following command `brew install cmake`
on macOS there is 2 ways to setup a build, using makefile or Xcode
```
cd Play/build_cmake
mkdir build
cd build
```
```
# Not specifying -G would automatically pick Makefiles
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
# OR
cmake .. -G"Xcode"
cmake --build . --config Release
```

### Building for UNIX ###
if you dont have cmake or openal lib installed, you can install it using your OS packaging tool, e.g ubuntu `apt install cmake libalut-dev`
on UNIX systems there is 2 ways to setup a build, using makefile or Ninja
```
cd Play/build_cmake
mkdir build
cd build
```
```
# Not specifying -G would automatically pick Makefiles
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/qt56/
cmake --build .
# OR
cmake .. -G"Ninja" -DCMAKE_PREFIX_PATH=/opt/qt56/
cmake --build . --config Release
```
Note `CMAKE_PREFIX_PATH` refers to the qt directory containing bin/libs folder, the above example uses a backport repo to install qt5.6 on trusty, if you install qt from their offical website, your `CMAKE_PREFIX_PATH` might look like this `~/Qt5.6.0/5.6/gcc_64/`

### Building for Android ###

Building for Android has been tested on macOS and UNIX environments.
Android can be built using Android Studio or through Gradle.

- Android Studio:
Files->Open Projects->Directory To Play/build_android
Install NDK using sdk manager
edit/create Play/build_android/local.properties, this file should contain sdk.dir when opened through Android Studio
for OSX: on a new line add `ndk.dir=/Users/USER_NAME/Library/Android/sdk/ndk-bundle` replacing `USER_NAME` with your macOS username,
for UNIX: on a new line add `ndk.dir=~/Android/Sdk/ndk-bundle`
for Windows: ##### TODO #####
note, this line would only be valid if you installed NDK through Android Studio's SDK manager.
Once this is done, you can start the build

- Gradle
edit/create Play/build_android/local.properties, this file should contain `sdk.dir` when opened through Android Studio, otherwise it wont exist and you can create it. on a new line add `ndk.dir=/location/to/ndk`, if you installed the NDK using android SDK it should be in `/Users/USER_NAME/Library/Android/sdk/ndk-bundle` replacing `USER_NAME` with your macOS username, else you can download it manually and set the path as appropriate.
```
cd Play/build_android
sh gradle assembleDebug
```
##### TODO: add instructions about Release/Signed builds. #####

### Building for iOS ###

Building for iOS has been tested with Xcode 6 and Xcode 7. 

To build for those platforms, you need to first build boost using the [script](https://github.com/jpd002/Play-Dependencies/blob/master/BoostMac/boost.sh) provided in the [Dependencies](https://github.com/jpd002/Play-Dependencies) repository. This will create the boost Xcode framework files that are needed by the projects from this repository. Once this is done, you will be able to open `Play.xcodeproj` for either OSX and iOS and build the project normally.
