:Compiles static 64 bit single exe build of AFISync with MSVC
set QT_STATIC_BINS=E:\qts\bin
set OPENSSL_ROOT_DIR=E:\afisync2\openssl-3.4.1
set VS_DIR=E:\Program Files\Microsoft Visual Studio\2022\Community

set ROOT_DIR=E:\afisync2
set SRC_DIR=%ROOT_DIR%\pbocheck
set SRC_BIN=%SRC_DIR%\bin
set PATH=%QT_STATIC_BINS%;%VS_DIR%\VC\Auxiliary\Build;%ROOT_DIR%;%systemroot%;%systemroot%\System32;%SRC_BIN%;E:\unison\bin;C:\Windows\System32\OpenSSH;E:\cygwin64\bin

mkdir build
cd build

call vcvarsall.bat x86_amd64

cmake .. -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles"
nmake
