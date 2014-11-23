#!/bin/bash
set -e
$ANDROID_NDK_ROOT/ndk-build clean
$ANDROID_NDK_ROOT/ndk-build clean NDK_DEBUG=1
$ANT_HOME/bin/ant clean
