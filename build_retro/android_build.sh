#!/usr/bin/env bash

set -e
set -x

if [ -z "$ANDROID_NDK" ]
then
	echo "Please set ANDROID_NDK and run again"
	exit -1
fi

STRIP="${ANDROID_NDK}/toolchains/llvm/prebuilt/*/bin/llvm-strip"
ABI_LIST="arm64-v8a armeabi-v7a x86 x86_64"
for ABI in $ABI_LIST
do
	mkdir "build_$ABI"
	pushd "build_$ABI"
	cmake ../.. -DBUILD_LIBRETRO_CORE=yes -DBUILD_PLAY=off \
	-GNinja \
	-DANDROID_ABI="${ABI}" \
	-DANDROID_NDK=${ANDROID_NDK} \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_FILE} \
	-DANDROID_NATIVE_API_LEVEL=19 \
	-DANDROID_STL=c++_static \
	-DANDROID_TOOLCHAIN=clang \
	-DCMAKE_MAKE_PROGRAM=${NINJA_EXE}
	
	cmake --build . --target play_libretro
	${STRIP} --strip-all -o ../play_libretro_${ABI}_android.so Source/ui_libretro/play_libretro_android.so
	popd
done