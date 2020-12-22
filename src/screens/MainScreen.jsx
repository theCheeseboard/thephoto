import React from 'react';

import WsController from '../WsController'

import Header from './Header';
import CameraScreen from './CameraScreen';

class MainScreen extends React.Component {
    constructor(props) {
        super(props);
        
        this.state = {
            page: "camera"
        }
    }
    
    changePage(page) {
        this.setState({
            page: page
        });
    }
    
    exit() {
        WsController.disconnect();
    }
    
    render() {
        return <div className="mainWrapper">
            <Header page={this.state.page} onChangePage={this.changePage.bind(this)} onExit={this.exit.bind(this)} />
            {this.renderPage()}
        </div>
    }
    
    renderPage() {
        switch (this.state.page) {
            case "camera":
                return <CameraScreen />
            case "transfers":
                return <span>This is the Transfers page</span>
            case "upload":
                return <span>This is the Upload page</span>
        }
    }
}

export default MainScreen;