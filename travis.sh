if [ $STAGE = "script" ]; then
  if [ $TRAVIS_OS_NAME = "linux" ]; then
    echo "[TRAVIS] Preparing build environment"
    source /opt/qt512/bin/qt512-env.sh
    echo "[TRAVIS] Building and installing the-libs"
    git clone https://github.com/vicr123/the-libs.git
    cd the-libs
    git checkout blueprint
    qmake
    make
    sudo make install INSTALL_ROOT=/
    cd ..
    echo "[TRAVIS] Running qmake"
    qmake
    echo "[TRAVIS] Building project"
    make
    echo "[TRAVIS] Installing into appdir"
    make install INSTALL_ROOT=~/appdir
    echo "[TRAVIS] Getting linuxdeployqt"
    wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod a+x linuxdeployqt-continuous-x86_64.AppImage
    echo "[TRAVIS] Building AppImage"
    ./linuxdeployqt-continuous-x86_64.AppImage ~/appdir/usr/share/applications/*.desktop -bundle-non-qt-libs -extra-plugins=iconengines/libqsvgicon.so,imageformats/libqsvg.so
    ./linuxdeployqt-continuous-x86_64.AppImage ~/appdir/usr/share/applications/*.desktop -appimage -extra-plugins=iconengines/libqsvgicon.so,imageformats/libqsvg.so
  else
    echo "[TRAVIS] Building for macOS"
    QT_VERSION=$(brew info qt | head -n 1 | cut -d' ' -f3)
    if [ "$TRAVIS_BRANCH" = "blueprint" ]; then
      THEPHOTO_APPPATH="thePhoto Blueprint.app"
      CONFIG_JSON=./node-appdmg-config-bp.json
    else
      THEPHOTO_APPPATH=thePhoto.app
      CONFIG_JSON=./node-appdmg-config.json
    fi
    export PATH="/usr/local/opt/qt/bin:$PATH"
    cd ..
    echo "[TRAVIS] Building and installing the-libs"
    git clone https://github.com/vicr123/the-libs.git
    cd the-libs
    git checkout blueprint
    qmake
    make
    sudo make install INSTALL_ROOT=/
    cd ..
    echo "[TRAVIS] Building and installing Contemporary"
    git clone https://github.com/vicr123/contemporary-theme.git
    cd contemporary-theme
    qmake
    make
    cd ..
    mkdir "build-thephoto"
    cd "build-thephoto"
    echo "[TRAVIS] Running qmake"
    qmake "INCLUDEPATH += /usr/local/opt/qt/include" "LIBS += -L/usr/local/opt/qt/lib" ../thePhoto/thePhoto.pro
    echo "[TRAVIS] Building project"
    make
    echo "[TRAVIS] Embedding the-libs"
    mkdir "$THEPHOTO_APPPATH/Contents/Libraries"
    cp /usr/local/lib/libthe-libs*.dylib "$THEPHOTO_APPPATH/Contents/Libraries/"
    install_name_tool -change libthe-libs.1.dylib @executable_path/../Libraries/libthe-libs.1.dylib "$THEPHOTO_APPPATH/Contents/MacOS/thePhoto"*
    install_name_tool -change @rpath/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets "$THEPHOTO_APPPATH/Libraries/libthe-libs.1.dylib"
    install_name_tool -change @rpath/QtWidgets.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui "$THEPHOTO_APPPATH/Libraries/libthe-libs.1.dylib"
    install_name_tool -change @rpath/QtWidgets.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore "$THEPHOTO_APPPATH/Libraries/libthe-libs.1.dylib"
    echo "[TRAVIS] Deploying Qt Libraries"
    macdeployqt "$THEPHOTO_APPPATH"
    echo "[TRAVIS] Embedding QtDBus"
    cp -r /usr/local/opt/qt/lib/QtDBus.framework "$THEPHOTO_APPPATH/Contents/Frameworks/"
    install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore "$THEPHOTO_APPPATH/Contents/Frameworks/QtDBus.framework/Versions/5/QtDBus"
    echo "[TRAVIS] Deploying Contemporary"
    cp ../contemporary-theme/libContemporary.dylib "$THEPHOTO_APPPATH/Contents/Plugins/styles/"
    install_name_tool -change libthe-libs.1.dylib @executable_path/../Libraries/libthe-libs.1.dylib "$THEPHOTO_APPPATH/Contents/Plugins/styles/libContemporary.dylib"
    install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets "$THEPHOTO_APPPATH/Contents/Plugins/styles/libContemporary.dylib"
    install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui "$THEPHOTO_APPPATH/Contents/Plugins/styles/libContemporary.dylib"
    install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore "$THEPHOTO_APPPATH/Contents/Plugins/styles/libContemporary.dylib"
    install_name_tool -change /usr/local/opt/qt/lib/QtDBus.framework/Versions/5/QtDBus @executable_path/../Frameworks/QtDBus.framework/Versions/5/QtDBus "$THEPHOTO_APPPATH/Contents/Plugins/styles/libContemporary.dylib"
    echo "[TRAVIS] Preparing Disk Image creator"
    npm install appdmg@0.5.2
    echo "[TRAVIS] Building Disk Image"
    ./node_modules/appdmg/bin/appdmg.js $CONFIG_JSON ~/thePhoto-macOS.dmg
  fi
elif [ $STAGE = "before_install" ]; then
  if [ $TRAVIS_OS_NAME = "linux" ]; then
    wget -O ~/vicr12345.gpg.key https://vicr123.com/repo/apt/vicr12345.gpg.key
    sudo apt-key add ~/vicr12345.gpg.key
    sudo add-apt-repository 'deb https://vicr123.com/repo/apt/ubuntu bionic main'
    sudo add-apt-repository -y ppa:beineri/opt-qt-5.12.3-xenial
    sudo apt-get update -qq
    sudo apt-get install qt512-meta-minimal qt512x11extras qt512tools qt512translations qt512svg qt512websockets qt512multimedia xorg-dev libxcb-util0-dev libgl1-mesa-dev
  else
    echo "[TRAVIS] Preparing to build for macOS"
    brew update
    brew install qt5
    brew upgrade node
  fi
elif [ $STAGE = "after_success" ]; then
  if [ $TRAVIS_OS_NAME = "linux" ]; then
    echo "[TRAVIS] Publishing AppImage"
    wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
    cp thePhoto*.AppImage thePhoto-linux.AppImage
    cp thePhoto*.AppImage.zsync thePhoto-linux.AppImage.zsync
    bash upload.sh thePhoto-linux*.AppImage*
  else
    echo "[TRAVIS] Publishing Disk Image"
    cd ~
    wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
    bash upload.sh thePhoto-macOS.dmg
  fi
fi
