#!/bin/sh
set -e
SCRIPT_PATH=$(dirname "$0")
APP_PATH=$SCRIPT_PATH/build/Source/ui_qt/Release/Play.app
ZIP_PATH=$SCRIPT_PATH/Play.zip
ditto -ck --sequesterRsrc --keepParent $APP_PATH $ZIP_PATH
xcrun altool --notarize-app -f $ZIP_PATH -t osx -u $MACOS_NOTARIZE_APPLEID_USERNAME -p $MACOS_NOTARIZE_APPLEID_PASSWORD --primary-bundle-id com.virtualapplications.play
