import React from 'react';

import WsController from '../WsController'

import Header from './Header';
import CameraScreen from './CameraScreen';
import TransfersScreen from './TransfersScreen';
import UploadScreen from './UploadScreen';

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
                return <TransfersScreen />
            case "upload":
                return <UploadScreen onUploadSelected={this.onUploadSelected.bind(this)}/>
            default:
                return <span>Please navigate to another page</span>
        }
    }

    onUploadSelected() {
        this.changePage("transfers");
    }
}

export default MainScreen;