@echo off
CALL %ANDROID_SDK_ROOT%/tools/android.bat update project -p .
%ANT_HOME%/bin/ant clean
