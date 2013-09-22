mkdir output
pushd output
mkdir coff
mkdir macho
pushd macho
mkdir armv7
mkdir i386
popd
popd
"..\build_win32\x64\ReleaseAot\PsfAot.exe" compile ".\blocks" x86 coff .\output\coff\PsfBlocks.obj
"..\build_win32\x64\ReleaseAot\PsfAot.exe" compile ".\blocks" x86 macho .\output\macho\i386\PsfBlocks.o
"..\build_win32\x64\ReleaseAot\PsfAot.exe" compile ".\blocks" arm macho .\output\macho\armv7\PsfBlocks.o
@pause
