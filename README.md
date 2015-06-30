# Play!

Play! is an attempt to create a PlayStation 2 emulator for Windows, OS X, Android & iOS platforms.

Ongoing compatibility list can be found here: [Compatibility List Wiki](https://github.com/jpd002/Play-/wiki/Compatible-games).

For more information, please visit [purei.org](http://purei.org).

## Project Dependencies ##

### External Libraries ###
- [boost](http://boost.org)

### Repositories ###
- [Play! Dependencies](https://github.com/jpd002/Play-Dependencies)
- [Play! Framework](https://github.com/jpd002/Play--Framework) 
- [Play! CodeGen](https://github.com/jpd002/Play--CodeGen)

## Building ##

### General Setup ###

Make sure your working copies share the same parent folder. Your setup should look like this:

C:\Projects
- CodeGen
- Dependencies
- Framework
- Play


### Building for Android ###

Building for Android has only been tested under Cygwin, but should work on other UNIX-like environments.

- Make a copy of `ExternalDependencies.mk.template` found in `build_android/jni` and rename to `ExternalDependencies.mk`
- Open the newly copied `ExternalDependencies.mk` and change paths inside to point to the proper dependency/repository paths (ie.: `/path/to/CodeGen` -> `/cygdrive/c/ProjectsGit/CodeGen/`)
- The build script relies on some environment variables that must be set before building:
	- `ANDROID_NDK_ROOT` -> Must refer to the Android NDK's path (ie.: `/cygdrive/c/Android/android-ndk-r10d`)
	- `ANDROID_SDK_ROOT` -> Must refer to the Android SDK's path (ie.: `/cygdrive/c/Android/android-sdk`)
	- `ANT_HOME` -> Must refer to a valid Apache Ant installation.
- Make sure you've built all necessary depencendies: boost, Framework and CodeGen.
- Run the `build_debug.sh` script available in the `build_android` directory to generate a debug build and `build_release.sh` for a release build.
