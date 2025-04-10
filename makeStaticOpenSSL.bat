:Compiles static 64 version of OpenSSL
set PATH=E:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build;%systemroot%;%systemroot%\System32
call vcvarsall.bat x86_amd64
cd ../openssl-3.4.1
"../strawberry-perl-5.38.2.2-64bit-portable/perl/bin/perl.exe" Configure VC-WIN64A no-shared
nmake