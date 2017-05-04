@echo off
mkdir build_debugger
pushd build_debugger
cmake .. -G "Visual Studio 15 2017 Win64" -DDEBUGGER_INCLUDED=on
popd
