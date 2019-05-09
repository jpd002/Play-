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
set PSFAOT_PATH=..\..\..\build_cmake\build_psfaot\tools\PsfPlayer\Source\ui_aot\RelWithDebInfo
"%PSFAOT_PATH%\PsfAot.exe" compile ".\blocks" x86 coff .\output\coff\PsfBlocks.obj
"%PSFAOT_PATH%\PsfAot.exe" compile ".\blocks" x86 macho .\output\macho\i386\PsfBlocks.o
"%PSFAOT_PATH%\PsfAot.exe" compile ".\blocks" arm macho .\output\macho\armv7\PsfBlocks.o
"%PSFAOT_PATH%\PsfAot.exe" compile ".\blocks" arm64 macho .\output\macho\arm64\PsfBlocks.o
@pause
