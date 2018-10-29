if [ $STAGE = "beforeinstall" ]; then
  yes | sdkmanager "platforms;android-27";
elif [ $STAGE = "script" ]; then
  ./gradlew assembleRelease;
  cp app/build/outputs/apk/release/app-release.apk ~/thePhoto-android.apk
elif [ $STAGE = "after_success" ]; then
  echo "[TRAVIS] Publishing APK"
  cd
  wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
  bash upload.sh thePhoto-android.apk
fi
