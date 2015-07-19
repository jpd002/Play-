#!/bin/bash
set -e
$ANDROID_SDK_ROOT/tools/android.bat update project -p .
$ANT_HOME/bin/ant debug -Dndk.debug=1
