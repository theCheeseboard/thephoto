if [ $STAGE = "before_install" ]; then
  openssl aes-256-cbc -K $encrypted_ed87c803ec54_key -iv $encrypted_ed87c803ec54_iv -in secrets.tar.enc -out secrets.tar -d
  tar -xvf secrets.tar
  cp google-play.json ~/
elif [ $STAGE = "script" ]; then
  ls keys.jks
  
  ./gradlew assembleRelease;
  cp app/build/outputs/apk/release/app-release.apk ~/thePhoto-android.apk
  cp thephotoeventwear/build/outputs/apk/release/thephotoeventwear-release.apk ~/thePhoto-wearos.apk
elif [ $STAGE = "after_success" ]; then
  echo "[TRAVIS] Publishing APK"
  cd
  wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
  bash upload.sh thePhoto-android.apk
  bash upload.sh thePhoto-wearos.apk
  if [ $TRAVIS_BRANCH = "android-blueprint" ]; then
    echo "[TRAVIS] Pushing to Google Play"
    gem install fastlane
    fastlane supply --apk thePhoto-android.apk --track beta --json_key google-play.json --package_name com.vicr123.thephotoevent
    fastlane supply --apk thePhoto-wearos.apk --track beta --json_key google-play.json --package_name com.vicr123.thephotoevent
  fi
fi
