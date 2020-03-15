import * as RNLocalize from "react-native-localize";
import i18next from 'i18next';
import { initReactI18next } from 'react-i18next';

const languages = {
    en: require("../translations/en.json"),
    vi: require("../translations/vi.json")
}

const backend = {
    type: 'backend',
    init: function() {},
    read: function(language, namespace, callback) {
        callback(null, languages[language]);
    }
}

const languageDetector = {
  type: 'languageDetector',
  detect: function() {
      console.log(RNLocalize.getLocales()[0].languageCode);
      return RNLocalize.getLocales()[0].languageCode;
  },
  init: () => {},
  cacheUserLanguage: () => {},
};

let trFunction;

i18next.use(backend).use(languageDetector).use(initReactI18next).init({
    fallbackLng: 'en',
    defautNs: "translation"
});

export default i18next;