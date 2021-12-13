import React from 'react';

import WsController from './WsController'

class Welcome extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            room: window.localStorage.getItem("room") || "",
            username: window.localStorage.getItem("username") || "",
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

        let room = this.state.room.substr(0, 4);
        let hmac = this.state.room.substr(4);
        
        WsController.connect(room, this.state.username, hmac).catch((error) => {
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
        let onRoomChange = e => {
            window.localStorage.setItem("room", e.target.value);
            this.setState({room: e.target.value});
        }

        let onUsernameChange = e => {
            window.localStorage.setItem("username", e.target.value);
            this.setState({username: e.target.value});
        }

        return <div className="welcomeWrapper">
            <div className="welcomeContainer">
                <h1>Welcome to thePhoto</h1>
                <p>For the best experience, grab the app:</p>
                <hr />
                <h2>Connect</h2>
                <p>You can also connect using your web browser.</p>
                
                <div className="welcomeForm">
                    <span>Room</span>
                    <input type="text" value={this.state.room} onChange={onRoomChange}/>
                    
                    <span>Username</span>
                    <input type="text" value={this.state.username} onChange={onUsernameChange}/>
                    
                    {this.renderError()}
                    <button onClick={this.connect.bind(this)}>Connect</button>
                </div>
            </div>
        </div>
    }
}

export default Welcome;