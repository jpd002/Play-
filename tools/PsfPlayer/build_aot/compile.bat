mkdir output
pushd output
mkdir coff
mkdir macho
pushd macho
mkdir i386
mkdir armv7
mkdir arm64
popd
popd
"..\build_win32\x64\ReleaseAot\PsfAot.exe" compile ".\blocks" x86 coff .\output\coff\PsfBlocks.obj
"..\build_win32\x64\ReleaseAot\PsfAot.exe" compile ".\blocks" x86 macho .\output\macho\i386\PsfBlocks.o
"..\build_win32\x64\ReleaseAot\PsfAot.exe" compile ".\blocks" arm macho .\output\macho\armv7\PsfBlocks.o
"..\build_win32\x64\ReleaseAot\PsfAot.exe" compile ".\blocks" arm64 macho .\output\macho\arm64\PsfBlocks.o
@pause
