#!/bin/sh
set -e
SCRIPT_PATH=$(dirname "$0")
APP_PATH=$SCRIPT_PATH/build/Source/ui_qt/Release/Play.app
ZIP_PATH=$SCRIPT_PATH/Play.zip
ditto -ck --sequesterRsrc --keepParent $APP_PATH $ZIP_PATH
xcrun notarytool submit $ZIP_PATH --apple-id $MACOS_NOTARIZE_APPLEID_USERNAME --team-id $MACOS_NOTARIZE_TEAMID --password $MACOS_NOTARIZE_APPLEID_PASSWORD --wait
