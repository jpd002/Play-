#!/bin/bash
set -e
$ANDROID_SDK_ROOT/tools/android.bat update project -p .
$ANT_HOME/bin/ant clean -Dndk.debug=0
$ANT_HOME/bin/ant clean -Dndk.debug=1

