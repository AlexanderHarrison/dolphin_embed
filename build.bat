set OUT=main.exe
set FILES=src\main.c

set BASE_FLAGS=/O2 /EHsc /Fo%OUT% /Fe%OUT%
set WARN_FLAGS=/W3 /WX
set INCLUDE_PATHS="C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um\x64\"
set LIBRARY_PATHS="C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x64\"

set LIBRARIES=raylib\lib\raylib.lib opengl32.lib msvcrt.lib kernel32.lib user32.lib gdi32.lib winmm.lib

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cl %FILES% && link /OUT:%OUT% main.obj %LIBRARIES%
