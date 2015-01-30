#!/bin/bash
set -e
$ANDROID_SDK_ROOT/tools/android.bat update project -p .
$ANDROID_NDK_ROOT/ndk-build
$ANT_HOME/bin/ant release
