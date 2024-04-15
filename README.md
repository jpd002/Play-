# Play! #
Play! is a PlayStation2 emulator for Windows, macOS, UNIX, Android, iOS & web browser platforms.

Compatibility information is available on the official [Compatibility Tracker](https://github.com/jpd002/Play-Compatibility).
If a specific game doesn't work with the emulator, please create a new issue there.

For more information, please visit [purei.org](https://purei.org).

You can try the experimental web browser version here: [playjs.purei.org](https://playjs.purei.org).

For general discussion, you're welcome to join our Discord: https://discord.gg/HygQnQP.

## Command Line Options (Windows/macOS/Linux) ##

The following command line options are available:
- `--disc "disc image path"` : Boots a disc image.
- `--elf "elf file path"` : Boots a ELF file.
- `--arcade "arcade id"` : Boots an arcade game.
- `--state "slot number"` : Loads a state from slot number.
- `--fullscreen` : Starts the emulator in fullscreen mode.

## Running on iOS ##

This emulator uses JIT code generation to speed things up. This is not supported by default by iOS, thus, there are some extra requirements:

- Have a device running iOS 13 or less, or an arm64e device running iOS 14.2/14.3.
- Have a jailbroken device.

If these requirements are not met, there are still ways to enable JIT through other means. Here is a guide explaining how JIT can be enabled:

https://spidy123222.github.io/iOS-Debugging-JIT-Guides/

Play! implements automatic JIT activation through AltServer, which requires AltServer to be running on the same network as your iOS device. This can be enabled in the Settings menu of the emulator.

You can also build the emulator yourself and launch it through Xcode's debugger to enable JIT. This will require installing the iOS SDK. If you don't want to be tethered to Xcode, you can use the "Detach" button in the "Debug" section after attaching the debugger to the app proccess. After you detach the debugger, the debug process will stay on the app until the app's process ends.

**If you try to play a game without JIT enabled, you will experience a crash when you launch the game.**

### Adding games to the library ###

Please refer to the following guides for instructions regarding adding your disc images to the Play! application on your iOS device:
- https://support.apple.com/HT201301
- https://support.apple.com/HT210598

If your device is jailbroken, the emulator will look through all bootable files in the `/private/var/mobile` directory and all of its subdirectories.

## Namco System 2x6 Arcade Support ##

### Placing dongle images and disc images ###

The files required to run arcade games should be placed inside the `arcaderoms` subdirectory of your `Play! Data Files` directory.

```
arcaderoms/
  bldyr3b.zip
  bldyr3b/
    bldyr3b.chd
  tekken4.zip
  tekken4/
    tef1dvd0.chd  
```

## Arcade Specific Controls ##

Some arcade specific actions are mapped to these buttons on the PS2 controller mappings:

- Service/Coin: SELECT
- Test: L3 & R3 pressed at the same time

### Light Gun Support ###

For games that support light guns, the following buttons are mapped:

- Gun Trigger: CIRCLE
- Pedal: TRIANGLE

The mouse's cursor position on the emulator's window will be used for the gun's position. It's also possible to map mouse buttons to CIRCLE or TRIANGLE in controller settings for a better experience.

**Note for Time Crisis 3**: This game requires prior calibration of the light gun in service menu. Hold the Test buttons, go in "I/O Test" then "Gun Initialize" and press the Pedal button to calibrate the gun (shoot at the center). This only needs to be done once.

### Taiko Drum Support ###

For Taiko no Tatsujin games, the following buttons are mapped:

- Left Men (面) : L1
- Left Fuchi (ふち) : L2
- Right Men (面) : R1
- Right Fuchi (ふち) : R2

### Driving Support ###

For driving games, the following buttons are mapped:

- Wheel : Left Analog Stick X +/-
- Gaz Pedal : Left Analog Stick Y +
- Brake Pedal : Right Analog Stick X +

## General Troubleshooting ##

#### Failed to open CHD file ####

Please make sure your CHD files are in the proper format. It's possible to use `chdman` to verify whether your CDVD image is really a CDVD image.

```
chdman info -i image.chd
```

If you see a `GDDD` metadata in there, it means your CDVD image needs to be converted. It can be done this way:

```
mv image.chd image.chd.orig
chdman extracthd -i image.chd.orig -o image.iso
chdman createcd -i image.iso -o image.chd
```

## Building ##

### Getting Started ###
First you'll need to clone this repo which contains the emulator source code, alongside the submodules required to build Play!:
 ```
 git clone --recurse-submodules https://github.com/jpd002/Play-.git
 cd Play-
 ```

### Building for Windows ###
The easiest way to build the project on Windows is to open Qt Creator and direct it to the Cmake file in `/project/dir/Play-/CMakeLists.txt`.
You can also build the project using Visual Studio or cmdline, for that you must follow these instructions:

To build for Windows you will need to have CMake installed on your system.
 ```cmd
 mkdir build
 cd build
 ```
 ```
 # Not specifying -G will automatically generate 32-bit projects.
 cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_PREFIX_PATH="C:\Qt\5.10.1\msvc2017_64" -DUSE_QT=YES
 ```
You can now build the project by opening the generated Visual Studio Solution or continue through cmdline:
 ```cmd
 cmake --build . --config Release
 ```
Note: `--config` can be `Release`, `Debug`, or `RelWithDebInfo`.

### Building for macOS & iOS ###
If you don't have CMake installed, you can install it using [Homebrew](https://brew.sh) with the following command:
 ```bash
 brew install cmake
 ```

There are two ways to generate a build for macOS. Either by using Makefiles, or Xcode:
 ```bash
 mkdir build
 cd build
 ```
 ```
 # Not specifying -G will automatically pick Makefiles
 cmake .. -G Xcode -DCMAKE_PREFIX_PATH=~/Qt/5.1.0/clang_64/
 cmake --build . --config Release
 # OR
 cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=~/Qt/5.1.0/clang_64/
 cmake --build .
 ```
To generate a build for iOS, you will need to add the following parameters to the CMake invocation:
 ```bash
 -DCMAKE_TOOLCHAIN_FILE=../../../Dependencies/cmake-ios/ios.cmake -DTARGET_IOS=ON
 ```

iOS build doesn't use Qt, so omit `-DCMAKE_PREFIX_PATH=...`

Example:
 ```bash
 cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../deps/Dependencies/cmake-ios/ios.cmake -DTARGET_IOS=ON
 ```

Note: iOS builds generated with Makefiles will not be FAT binaries.

To test your iOS builds on a device, you will need to setup code signing:
- Set `CODE_SIGNING_ALLOWED` to `YES` on the `Play` target.
- Set your code signing parameters in Signing & Capabilities tab in Xcode.

To build with Vulkan on macOS, just make sure the `$VULKAN_SDK` environment variable is set with the proper path.

On iOS, you will need to add this to your CMake command line:
 ```bash
 -DCMAKE_PREFIX_PATH=$VULKAN_SDK
 ```

### Building for UNIX ###
if you don't have Cmake or OpenAL lib installed, you'll also require Qt. (preferably version 5.6)
You can install it using your OS packaging tool, e.g Ubuntu: `apt install cmake libalut-dev qt5-default libevdev-dev libqt5x11extras5-dev libsqlite3-dev`

On UNIX systems there are 3 ways to setup a build. Using Qt creator, makefile or Ninja:
 - QT Creator
    - Open Project -> `Play/CMakeLists.txt`

 - Makefile/Ninja:
   ```bash
   mkdir build
   cd build
   ```
   ```
   # Not specifying -G will automatically pick Makefiles
   cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/qt56/
   cmake --build .
   # OR
   cmake .. -G Ninja -DCMAKE_PREFIX_PATH=/opt/qt56/
   cmake --build . --config Release
   ```
The above example uses a backport repo to install Qt5.6 on Ubuntu Trusty.

Note: `CMAKE_PREFIX_PATH` refers to the Qt directory containing bin/libs folder. If you install Qt from their official website, your `CMAKE_PREFIX_PATH` might look like this: `~/Qt5.6.0/5.6/gcc_64/`

### Building for Android ###
Building for Android has been tested on macOS and UNIX environments.

Android can be built using Android Studio, or Gradle:
 - Android Studio:
   - Files-> Open Projects-> Directory To `Play/build_android`
   - Install NDK using SDK manager
     - Edit/create `Play/build_android/local.properties`
     - OSX: Add a new line: `ndk.dir=/Users/USER_NAME/Library/Android/sdk/ndk-bundle` replacing `USER_NAME` with your macOS username
     - UNIX: Add a new line: `ndk.dir=~/Android/Sdk/ndk-bundle`
     - Windows: Add a new line: `C:\Users\USER_NAME\AppData\Local\Android\sdk\ndk-bundle`
     - Please leave an empty new line at the end of the file.

Note: These examples are only valid if you installed NDK through Android Studio's SDK manager.
Otherwise, you must specify the correct location to the Android NDK.

Once this is done, you can start the build:
 - Gradle: Prerequisite Android SDK & NDK (Both can be installed through Android Studio)
   - edit/create `Play/build_android/local.properties`
     - OSX:
       - Add a new line: `sdk.dir=/Users/USER_NAME/Library/Android/sdk` replacing `USER_NAME` with your macOS username
       - Add a new line: `ndk.dir=/Users/USER_NAME/Library/Android/sdk/ndk-bundle` replacing `USER_NAME` with your macOS username
     - UNIX:
       - Add a new line: `sdk.dir=~/Android/Sdk`
       - Add a new line: `ndk.dir=~/Android/Sdk/ndk-bundle`
     - Windows:
       - Add a new line: `sdk.dir=C:\Users\USER_NAME\AppData\Local\Android\sdk`
       - Add a new line: `ndk.dir=C:\Users\USER_NAME\AppData\Local\Android\sdk\ndk-bundle`
     - Please leave an empty new line at the end of the file.

Note: These examples are only valid if you installed NDK through Android Studio's SDK manager.
Otherwise you must specify the correct location to Android NDK.
Once this is done, you can start the build:
 ```bash
 cd Play/build_android
 sh gradlew assembleDebug
 ```

##### About Release/Signed builds #####
Building through Android Studio, you have the option to “Generate Signed APK”.

When building through Gradle, make sure these variables are defined in a `gradle.properties` file, either in your project directory or in your `GRADLE_USER_HOME` directory:

 ```
 PLAY_RELEASE_STORE_FILE=/location/to/my/key.jks
 PLAY_RELEASE_STORE_PASSWORD=mysuperhardpassword
 PLAY_RELEASE_KEY_ALIAS=myalias
 PLAY_RELEASE_KEY_PASSWORD=myevenharderpassword
 ```

Then, you should be able to use `assembleRelease` to generate a signed release build.
 ```
 cd Play/build_android
 sh gradlew assembleRelease
 # or on Windows
 gradlew.bat assembleRelease
 ```

### Building for Web Browsers ###

Building for the web browser environment requires [emscripten](https://emscripten.org/).

Use `emcmake` to generate the project:

```
mkdir build
cd build
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_PLAY=ON -DBUILD_PSFPLAYER=ON -DUSE_QT=OFF
```

Upon completion, you can build the JavaScript/WebAssembly files using this command line:

```
cmake --build . --config Release
```

This will generate the JavaScript glue code and the WebAssembly module required for the web application located in `js/play_browser`. Here's where the output files from emscripten need to be copied:

```
build_cmake/build/Source/ui_js/Play.js -> js/play_browser/src/Play.js
build_cmake/build/Source/ui_js/Play.wasm -> js/play_browser/public/Play.wasm
build_cmake/build/Source/ui_js/Play.js -> js/play_browser/public/Play.js
build_cmake/build/Source/ui_js/Play.worker.js -> js/play_browser/public/Play.worker.js
```

Once this is done, you should be able to go in the `js/play_browser` folder and run the following to get a local version running in your web browser:

```
npm install
npm start
```

##### Browser environment caveats #####
- Write protection on memory pages is not supported, thus, games loading modules on the EE might not work properly since JIT cache can't be invalidated.
- No control over floating point environment, default rounding mode is used, which can cause issues in some games.
