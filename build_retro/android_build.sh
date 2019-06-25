#!/usr/bin/env bash

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
	-DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
	-DANDROID_NATIVE_API_LEVEL=23 \
	-DANDROID_TOOLCHAIN=clang
	
	cmake --build . --target play_libretro

	mv Source/ui_libretro/play_libretro.so ../play_libretro_${ABI}.so
	${STRIP} -strip-all ../play_libretro_${ABI}.so ../play_libretro_${ABI}_stripped.so
	popd
done