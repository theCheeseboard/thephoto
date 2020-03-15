import React from 'react';
import { View, StyleSheet } from 'react-native';

class HorizontalLayout extends React.Component {
    render() {
        return <View style={styles.horizontalLayout}>
            {this.props.children}
        </View>
    }
}

class VerticalLayout extends React.Component {
    render() {
        return <View style={styles.verticalLayout}>{this.props.children}</View>
    }
}

const styles = StyleSheet.create({
    horizontalLayout: {
        flexDirection: 'row',
        alignItems: 'center'
    },
    verticalLayout: {
        flexDirection: 'column',
        alignItems: 'center'
    }
});

export { VerticalLayout };
export { HorizontalLayout };