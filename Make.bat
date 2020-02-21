@setlocal
@call egcc.bat
@if not exist build32 mkdir build32
@set CFLAGS=%CFLAGS2%
@set CXXFLAGS=%CCFLAGS2%
@pushd build32
@%CMAKE%  ..\. -DCMAKE_BUILD_TYPE=Release
@mingw32-make
@popd
