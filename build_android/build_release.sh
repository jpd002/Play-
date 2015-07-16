#!/bin/bash
set -e
$ANDROID_SDK_ROOT/tools/android.bat update project -p .
$ANT_HOME/bin/ant release -Dndk.debug=0
