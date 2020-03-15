import React from 'react';
import {
  SafeAreaView,
  StyleSheet,
  View,
  Text,
  TextInput,
  Button,
  Switch,
  ActionSheetIOS,
  Platform,
  Alert,
  ActivityIndicator,
  PermissionsAndroid
} from 'react-native';

import { connectActionSheet } from '@expo/react-native-action-sheet';

import * as RNLocalize from "react-native-localize";
import { withTranslation } from 'react-i18next';
import { HorizontalLayout } from '../layout';
import { Navigation } from 'react-native-navigation';
import { ActionSheetProvider } from '@expo/react-native-action-sheet';

class StartScreen extends React.Component {
    constructor(props) {
        super(props);
        
        this.state = {
            code: '32322 ',
            keepImages: false,
            connecting: false
        }
    }
    
    componentDidUpdate(prevProps) {
        if (this.props.connectionError !== this.state.connectionError) {
            this.state.connectionError = this.props.connectionError;
            console.log("error");
            
            let buttons = [];
            if (Platform.OS === "android") {
                buttons.push({
                    text: this.props.t("START_SCREEN_CONNECTION_ERROR_WIFI_SETTINGS"),
                    onPress: () => {
                        
                    }
                });
            }
            buttons.push({text: this.props.t("OK")});
            
            Alert.alert(
                this.props.t("START_SCREEN_CONNECTION_ERROR_TITLE"),
                this.props.t("START_SCREEN_CONNECTION_ERROR_MESSAGE"),
                buttons,
                {
                    cancelable: true
                }
            );
        }
    }
    
    codeUpdated(text) {
        if (text.length > 5 && text[5] != ' ') text = text.substr(0, 5) + " " + text.substr(5);
        this.setState({
            code: text
        })
    }
    
    showMoreDialog() {
        this.props.showActionSheetWithOptions({
            options: [
                this.props.t("START_SCREEN_HELP"),
                this.props.t("START_SCREEN_SETTINGS"),
                this.props.t("CANCEL")
            ],
            message: this.props.t("START_SCREEN_MORE_MESSAGE"),
            cancelButtonIndex: 2
        }, (buttonIndex) => {
            switch (buttonIndex) {
                case 0: //Help
                    break;
                case 1: //Settings
                    Navigation.push(this.props.componentId, {
                        component: {
                            name: "navigation.SettingsScreen",
                            options: {
                                topBar: {
                                    title: {
                                        text: this.props.t("SETTINGS_SCREEN_TITLE")
                                    }
                                }
                            }
                        }
                    });
            }
        });
    }
    
    keepImagesDialog() {
        this.props.showActionSheetWithOptions({
            options: [
                this.props.t("START_SCREEN_KEEP_IMAGES"),
                this.props.t("START_SCREEN_NO_KEEP_IMAGES"),
                this.props.t("CANCEL")
            ],
            title: this.props.t("START_SCREEN_KEEP_IMAGES_TITLE"),
            message: this.props.t("START_SCREEN_KEEP_IMAGES_MESSAGE"),
            cancelButtonIndex: 2
        }, (buttonIndex) => {
            switch (buttonIndex) {
                case 0:
                    this.setState({
                        keepImages: true
                    });
                    break;
                case 1: 
                    this.setState({
                        keepImages: false
                    });
                    break;
            }
        });
    }
    
    async connect() {
        if (this.state.code.length !== 11) {
            Alert.alert(
                this.props.t("START_SCREEN_INVALID_CODE_TITLE"),
                this.props.t("START_SCREEN_INVALID_CODE_BODY"),
                [
                    {
                        text: this.props.t("OK")
                    }
                ],
                {
                    cancelable: true
                }
            );
            return;
        }
        
        //Check permissions on Android
        if (Platform.OS === "android") {
            try {
                let permissionsToAsk = [];
                if (!await PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.CAMERA)) permissionsToAsk.push(PermissionsAndroid.PERMISSIONS.CAMERA);
                if (this.state.keepImages && !await PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.WRITE_EXTERNAL_STORAGE)) permissionsToAsk.push(PermissionsAndroid.PERMISSIONS.WRITE_EXTERNAL_STORAGE);
                
                if (permissionsToAsk.length > 0) {
                    let allowed = await PermissionsAndroid.requestMultiple(permissionsToAsk);
                    if (!Object.values(allowed).every(result => result === PermissionsAndroid.RESULTS.GRANTED)) return;
                }
            } catch (err) {
                //Don't do anything
                console.log(err);
                return;
            }
        }
        
        this.props.onConnect(this.state.code);
    }
    
    mainView() {
        if (this.props.connecting) {
            return <View style={styles.centerView}>
                <Text style={{fontSize: 30, padding: 10, textAlign: 'center'}}>{this.props.t("THEPHOTO")}</Text>
                <HorizontalLayout style={{justifyContent: 'center', flexGrow: '1'}}>
                    <ActivityIndicator size="small" />
                    <Text>{this.props.t("START_SCREEN_CONNECTING")}</Text>
                </HorizontalLayout>
            </View>
        } else {
            return <View style={styles.centerView}>
                <Text style={{fontSize: 30, padding: 10, textAlign: 'center'}}>{this.props.t("THEPHOTO")}</Text>
                <TextInput value={this.state.code} 
                    onChangeText={this.codeUpdated.bind(this)} 
                    autoCompleteType="off"
                    autoCorrect={false}
                    autoFocus={true}
                    disableFullscreenUI={true}
                    importantForAutofill="no"
                    keyboardType="numeric"
                    maxLength={11}
                    placeholder={this.props.t("START_SCREEN_CODE")}
                    style={{paddingTop: 10, textAlign: 'center'}} />
                <HorizontalLayout>
                    <Button onPress={this.showMoreDialog.bind(this)} title={this.props.t("START_SCREEN_MORE")}/>
                    <View style={{flex: 1}} />
                    <Button onPress={this.connect.bind(this)} title={this.props.t("START_SCREEN_GO")}/>
                </HorizontalLayout>
                <HorizontalLayout>
                    <Button onPress={this.keepImagesDialog.bind(this)} title={this.state.keepImages ? this.props.t("START_SCREEN_KEEP_IMAGES") : this.props.t("START_SCREEN_NO_KEEP_IMAGES")} />
                </HorizontalLayout>
            </View>
        }
    }
    
    render() {
        return <SafeAreaView style={styles.mainContainer}>
                {this.mainView()}
            </SafeAreaView>
    }
}

const styles = StyleSheet.create({
    centerView: {
        flex: 0.8,
        padding: 20,
        flexDirection: 'column',
        alignItems: 'stretch',
        backgroundColor: 'white',
        borderRadius: 20,
        minHeight: 300
    },
    mainContainer: {
        flex: 1,
        flexDirection: 'row',
        justifyContent: 'center',
        alignItems: 'center',
        backgroundColor: 'rgb(150, 0, 75)'
    }
});

const ActionSheetStartScreen = withTranslation()(connectActionSheet(StartScreen));

export default props => <ActionSheetProvider>
        <React.Suspense fallback={<View />}>
            <ActionSheetStartScreen {...props} />
        </React.Suspense>
    </ActionSheetProvider>
