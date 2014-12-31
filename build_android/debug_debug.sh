#!/bin/bash
set -e
adb install -r ./bin/Play-debug.apk
$ANDROID_NDK_ROOT/ndk-gdb --nowait --start
