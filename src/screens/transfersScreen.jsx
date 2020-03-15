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

class TransfersScreen extends React.Component {
    constructor(props) {
        super(props);
        
    }
    
    transferItems() {
        let items = [];
        for (let i = 0; i < 10; i++) {
            items.push(<SettingsList.Item
                            title={`Item #`}
                        />)
        }
        return items;
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
                        <SettingsList.Header headerText={this.props.t("TRANSFERS_SCREEN_TITLE")} headerStyle={style.headerStyle}/>
                        {this.transferItems()}
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

export default withTranslation()(TransfersScreen);