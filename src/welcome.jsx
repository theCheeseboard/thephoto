import React from 'react';

import WsController from './WsController'

class Welcome extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            room: "",
            username: "",
            error: ""
        }
    }
    
    connect() {
        if (this.state.username === "") {
            this.setState({
                error: "Enter a username"
            });
            return;
        }
        
        WsController.connect(this.state.room, this.state.username).catch((error) => {
            this.setState({
                error: "Check the room code and try again"
            });
            return;
        });
    }
    
    renderError() {
        if (this.state.error === "") return null;
        return <span className="error">{this.state.error}</span>
    }
    
    render() {
        return <div className="welcomeWrapper">
            <div className="welcomeContainer">
                <h1>Welcome to thePhoto</h1>
                <p>For the best experience, grab the app:</p>
                <hr />
                <h2>Connect</h2>
                <p>You can also connect using your web browser.</p>
                
                <div class="welcomeForm">
                    <span>Room</span>
                    <input type="text" value={this.state.room} onChange={(e => this.setState({room: e.target.value}))}/>
                    
                    <span>Username</span>
                    <input type="text" value={this.state.username} onChange={(e => this.setState({username: e.target.value}))}/>
                    
                    {this.renderError()}
                    <button onClick={this.connect.bind(this)}>Connect</button>
                </div>
            </div>
        </div>
    }
}

export default Welcome;