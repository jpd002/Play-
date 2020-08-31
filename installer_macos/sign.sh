#!/bin/sh
set -e
SCRIPT_PATH=$(dirname "$0")
APP_PATH=$SCRIPT_PATH/../build/Source/ui_qt/Release/Play.app
codesign -s "Developer ID Application" -f $APP_PATH/Contents/Resources/libMoltenVk.dylib
codesign -s "Developer ID Application" -f --deep --entitlements $SCRIPT_PATH/Play.entitlements --options=runtime $APP_PATH
