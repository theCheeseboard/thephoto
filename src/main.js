import React from 'react';
import { Navigation } from 'react-native-navigation';
import i18n from './i18n'

import StartScreen from './screens/startScreen';
import SettingsScreen from './screens/settingsScreen';
import TransfersScreen from './screens/transfersScreen';

const CONTROLLER_STATE = {
    idle: 0,
    connecting: 1,
    ready: 2
};

class Controller {
    #stateVar;
    #ws;
    
    #startScreenExtraProps;
    
    constructor() {
        this.#startScreenExtraProps = {};
        
        this.#setState(CONTROLLER_STATE.idle);
    }
    
    connect(code) {
        if (this.#stateVar !== CONTROLLER_STATE.idle) return;
        
        this.#setState(CONTROLLER_STATE.connecting);
        
        let ipDec = parseInt(code.replace(" ", ""));
        let ip = [
            (ipDec & 0xFF000000) >>> 24,
            (ipDec & 0x00FF0000) >>> 16,
            (ipDec & 0x0000FF00) >>> 8,
            (ipDec & 0x000000FF)
        ]
        
        let addr = `ws://${ip[0]}.${ip[1]}.${ip[2]}.${ip[3]}:26158/`;
        this.#ws = new WebSocket(addr);
        this.#ws.onmessage = this.#wsReceive.bind(this);
        this.#ws.onopen = this.#wsReady.bind(this);
        this.#ws.onerror = this.#wsError.bind(this);
        console.log("Connecting");
    }
    
    close() {
        
    }
    
    #wsReady() {
        console.log("Handshaking");
        this.#ws.send(JSON.stringify({
            type: "handshake",
            name: "Your Name Here"
        }));
    }
    
    #wsReceive(e) {
        let data = JSON.parse(e.data);
        switch (this.#stateVar) {
            case CONTROLLER_STATE.connecting:
                if (data.status === "OK") {
                    this.#setState(CONTROLLER_STATE.ready);
                    
                    //Change screens
                    Navigation.push("startScreen", {
                        bottomTabs: {
                            id: "connectedScreen",
                            currentTabId: "transfersScreen",
                            children: [
                                {
                                    component: {
                                        id: "transfersScreen",
                                        name: "navigation.TransfersScreen",
                                        options: {
                                            text: i18n.t("TRANSFERS_SCREEN_TITLE"),
                                            icon: require("../android/app/src/main/res/mipmap-hdpi/ic_launcher.png")
                                        }
                                    }
                                },
                                {
                                    component: {
                                        id: "settingsScreen",
                                        name: "navigation.SettingsScreen",
                                        options: {
                                            text: i18n.t("SETTINGS_SCREEN_TITLE"),
                                            icon: require("../android/app/src/main/res/mipmap-hdpi/ic_launcher.png")
                                        }
                                    }
                                }
                            ]
                        }
                    })
                } else {
                    //An error occurred
                }
        }
    }
    
    #wsError(e) {
        this.#startScreenExtraProps.connectionError = e;
        this.#setState(CONTROLLER_STATE.idle);
        delete this.#startScreenExtraProps.connectionError;
    }
    
    #setState(state) {
        this.#stateVar = state;
        Navigation.updateProps("startScreen", {
            onConnect: this.connect.bind(this),
            connecting: this.isConnecting,
            ...this.#startScreenExtraProps
        });
    }
    
    get isConnecting() {
        return this.#stateVar === CONTROLLER_STATE.connecting;
    }
    
    get socket() {
        return this.#ws;
    }
}
let controller = new Controller;

function init() {
    Navigation.registerComponent("navigation.StartScreen", () => StartScreen);
    Navigation.registerComponent("navigation.SettingsScreen", () => SettingsScreen);
    Navigation.registerComponent("navigation.TransfersScreen", () => TransfersScreen);
    Navigation.events().registerAppLaunchedListener(async() => {
        Navigation.setRoot({
            root: {
                stack: {
                    children: [
                        {
                            component: {
                                id: "startScreen",
                                name: "navigation.StartScreen",
                                passProps: {
                                    onConnect: controller.connect.bind(controller),
                                    connecting: controller.isConnecting
                                },
                                options: {
                                    topBar: {
                                        visible: false,
                                        text: "thePhoto"
                                    }
                                }
                            }
                        }
                    ]
                }
            }
        })
    });
}

module.exports = init;