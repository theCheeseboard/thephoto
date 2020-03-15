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
  ActivityIndicator
} from 'react-native';

import * as RNLocalize from "react-native-localize";
import { withTranslation } from 'react-i18next';
import { HorizontalLayout } from '../layout';
import prompt from 'react-native-prompt-android';
import SettingsList from 'react-native-settings-list';

class SettingsScreen extends React.Component {
    constructor(props) {
        super(props);
        
        this.state = {
            displayName: "Your Name Here"
        };
    }
    
    changeDisplayName() {
        let name = prompt("Display Name", "", [
            {text: this.props.t("CANCEL"), style: 'cancel'},
            {text: this.props.t("OK"), onPress: name => {
                this.setState({
                    displayName: name
                });
            }}
        ], {
            defaultValue: this.state.displayName
        });
    }
    
    render() {
        let style;
        if (Platform.OS === 'ios') {
            style = {
                backgroundColor: "#F2F2F7",
                borderColor: "#C8C7CC",
                headerStyle: {
                    marginTop: 15,
                    marginLeft: 20,
                    color: "#939398",
                    textTransform: "uppercase"
                }
            };
        } else {
            style = {
                backgroundColor: "#F2F2F7",
                borderColor: "#C8C7CC",
                headerStyle: {
                    marginTop: 15,
                    marginLeft: 20,
                    color: "#939398",
                    textTransform: "uppercase"
                }
            };
        }
        
        return <SafeAreaView style={styles.mainContainer}>
                <View style={{flexGrow: 1, backgroundColor: style.backgroundColor}} >
                    <SettingsList borderColor={style.borderColor}>
                        <SettingsList.Header headerText={this.props.t("SETTINGS_SCREEN_GENERAL")} headerStyle={style.headerStyle}/>
                        <SettingsList.Item
                            title={this.props.t("SETTINGS_SCREEN_GENERAL_DISPLAY_NAME")}
                            titleInfo={this.state.displayName}
                            onPress={this.changeDisplayName.bind(this)}
                        />
                    </SettingsList>
                </View>
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
        justifyContent: 'flex-start',
        alignItems: 'flex-start'
    }
});

export default withTranslation()(SettingsScreen);