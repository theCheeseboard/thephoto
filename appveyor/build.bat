if "%APPVEYOR_REPO_TAG_NAME%"=="continuous" (
    exit 1
)

set QTDIR=C:\Qt\5.11\msvc2017_64
set PATH=%PATH%;%QTDIR%\bin
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
git clone https://github.com/vicr123/the-libs.git
cd the-libs
git checkout blueprint
qmake the-libs.pro
nmake release
nmake install
cd ..
qmake thePhoto.pro
nmake release
mkdir deploy
copy release\thephoto.exe deploy
copy "C:\Program Files\thelibs\lib\the-libs.dll" deploy
copy "C:\OpenSSL-Win64\bin\openssl.exe" deploy
copy "C:\OpenSSL-Win64\bin\libeay32.dll" deploy
copy "C:\OpenSSL-Win64\bin\ssleay32.dll" deploy
copy "C:\OpenSSL-Win64\bin\openssl.cfg" deploy
cd deploy
windeployqt thephoto.exe