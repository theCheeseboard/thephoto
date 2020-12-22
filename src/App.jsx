import React from 'react';
import './App.css';

import WsController from './WsController'

import Welcome from './welcome'
import MainScreen from './screens/MainScreen';

class App extends React.Component {
    constructor(props) {
        super(props);
        
        WsController.setWsStateChangeHandler(() => {
            this.setState({
                isConnected: WsController.isConnected()
            });
        });
        
        this.state = {
            isConnected: false
        }
    }
    
    renderMainPage() {
        if (this.state.isConnected) {
            return <MainScreen />
        } else {
            return <Welcome />
        }
    }
    
    render() {
        return <div className="App">
            {this.renderMainPage()}
        </div>
    }
}

export default App;
