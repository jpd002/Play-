#!/bin/bash
set -e
$ANDROID_SDK_ROOT/tools/android.bat update project -p .
$ANDROID_NDK_ROOT/ndk-build NDK_DEBUG=1
$ANT_HOME/bin/ant debug
