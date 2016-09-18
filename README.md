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

### Building for Android ###

Building for Android has been tested on Windows and UNIX environments.

- Make a copy of `ExternalDependencies.mk.template` found in `build_android/jni` and rename to `ExternalDependencies.mk`
- Open the newly copied `ExternalDependencies.mk` and change paths inside to point to the proper dependency/repository paths (ie.: `/path/to/boost` -> `C:\Libraries\Boost`)
- The build script relies on some environment variables that must be set before building:
	- `ANDROID_NDK` -> Must refer to the Android NDK's path (ie.: `C:\Android\android-ndk-r10e`)
	- `ANDROID_SDK_ROOT` -> Must refer to the Android SDK's path (ie.: `C:\Android\android-sdk`)
	- `ANT_HOME` -> Must refer to a valid Apache Ant installation.
- Make sure you've built all necessary depencendies: boost, Framework and CodeGen.
- Run the `build_debug` script available in the `build_android` directory to generate a debug build and `build_release` for a release build.

### Building for macOS and iOS ###

Building for macOS and iOS has been tested with Xcode 6 and Xcode 7. 

To build for those platforms, you need to first build boost using the [script](https://github.com/jpd002/Play-Dependencies/blob/master/BoostMac/boost.sh) provided in the [Dependencies](https://github.com/jpd002/Play-Dependencies) repository. This will create the boost Xcode framework files that are needed by the projects from this repository. Once this is done, you will be able to open `Play.xcodeproj` for either OSX and iOS and build the project normally.
