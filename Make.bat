setlocal
call egcc.bat
windres resource.rc -O coff -o resource.o
set LFLAGS=%LFLAGS% -Wl,--large-address-aware -mwindows
set LIBS=-lwin32hlp -lexshit -lstdshit -lcomdlg32 -luuid
gcc main.cc findcore.cc resource.o %CCFLAGS2% %LFLAGS% %LIBS% -o findText.exe
