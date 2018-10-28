if [ $STAGE = "beforeinstall" ]; then
  yes | sdkmanager "platforms;android-27";
elif [ $STAGE = "script" ]; then
  ./gradlew build connectedCheck;
fi
