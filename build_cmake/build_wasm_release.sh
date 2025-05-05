#!/bin/sh
script_dir=$(dirname -- "$0")
pushd ..
cmake --build --preset wasm-ninja-release
popd
cp $script_dir/build/wasm-ninja/Source/ui_js/Release/Play.js $script_dir/../js/play_browser/src/Play.js
cp $script_dir/build/wasm-ninja/Source/ui_js/Release/Play.js $script_dir/../js/play_browser/public/Play.js
cp $script_dir/build/wasm-ninja/Source/ui_js/Release/Play.wasm $script_dir/../js/play_browser/public/Play.wasm
cp $script_dir/build/wasm-ninja/tools/PsfPlayer/Source/ui_js/Release/PsfPlayer.js $script_dir/../js/psfplayer_browser/src/PsfPlayer.js
cp $script_dir/build/wasm-ninja/tools/PsfPlayer/Source/ui_js/Release/PsfPlayer.js $script_dir/../js/psfplayer_browser/public/PsfPlayer.js
cp $script_dir/build/wasm-ninja/tools/PsfPlayer/Source/ui_js/Release/PsfPlayer.wasm $script_dir/../js/psfplayer_browser/public/PsfPlayer.wasm
