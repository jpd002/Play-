#/bin/bash
set -e
mkdir -p Payload
rsync -a ../build/Source/ui_ios/Release-iphoneos/Play.app/* Payload/Play.app
zip -r Play.ipa Payload
