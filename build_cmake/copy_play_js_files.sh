#!/bin/sh
script_dir=$(dirname -- "$0")
cp $script_dir/build_wasm/Source/ui_js/Play.js $script_dir/../js/play_browser/src/Play.js
cp $script_dir/build_wasm/Source/ui_js/Play.js $script_dir/../js/play_browser/public/Play.js
cp $script_dir/build_wasm/Source/ui_js/Play.worker.js $script_dir/../js/play_browser/public/Play.worker.js
cp $script_dir/build_wasm/Source/ui_js/Play.wasm $script_dir/../js/play_browser/public/Play.wasm
