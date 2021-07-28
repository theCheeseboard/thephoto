import React from 'react';

import CameraHud from './CameraHud';
import Webcam from "react-webcam";
import { ReactComponent as CameraUnavailable } from "../assets/camera-unavailable.svg";

import Styles from './CameraScreen.module.css';

import WsController from '../WsController'

class CameraScreen extends React.Component {
    webcam;
    
    constructor(props) {
        super(props);
        
        this.webcam = React.createRef();
        
        this.state = {
            errorState: "noError",
            videoConstraints: {
                facingMode: "environment",
                width: 4096,
                height: 2160,
                frameRate: 60
            }
        }
    }
    
    renderPage() {
        switch (this.state.errorState) {
            case "noError":
                return [
                    <Webcam
                        key="webcam"
                        audio={false}
                        ref={this.webcam}
                        onUserMedia={this.mediaStream.bind(this)}
                        onUserMediaError={this.mediaError.bind(this)}
                        videoConstraints={this.state.videoConstraints}
                        screenshotFormat="image/png"
                        className={Styles.CameraWebcam} />,
                    <CameraHud
                        key="webcamHud"
                        onCapture={this.capture.bind(this)} />
                ]
            case "noCamera":
                return <div className={Styles.CameraError}>
                    <div className={Styles.CameraErrorContainer}>
                        <CameraUnavailable className={Styles.CameraErrorGlyph} />
                        <h1>There is no camera on this device.</h1>
                    </div>
                </div>
            case "noSupport":
                return <div className={Styles.CameraError}>
                    <div className={Styles.CameraErrorContainer}>
                        <CameraUnavailable className={Styles.CameraErrorGlyph} />
                        <h1>This browser is unsupported</h1>
                        <div>
                            <p>Use a browser that can access the camera. Possible options are:</p>
                            <ul>
                                <li>Google Chrome</li>
                                <li>Firefox</li>
                                <li>Safari</li>
                                <li>Microsoft Edge</li>
                            </ul>
                        </div>
                    </div>
                </div>
            case "noPermission":
                return <div className={Styles.CameraError}>
                    <div className={Styles.CameraErrorContainer}>
                        <CameraUnavailable className={Styles.CameraErrorGlyph} />
                        <h1>Can't access the camera</h1>
                        <div>
                            <p>Check the permissions for this page, and then reload the page to try again.</p>
                        </div>
                    </div>
                </div>
            case "genericError":
            default:
                return <div className={Styles.CameraError}>
                    <div className={Styles.CameraErrorContainer}>
                        <CameraUnavailable className={Styles.CameraErrorGlyph} />
                        <h1>Can't access the camera</h1>
                    </div>
                </div>
        }
    }
    
    render() {
        return <div className={Styles.CameraPage}>
            {this.renderPage()}
        </div>
    }
    
    mediaStream(event) {
        console.log(event);
    }
    
    mediaError(error) {
        console.log(error);
        if (error.message === "getUserMedia is not implemented in this browser") {
                this.setState({
                    errorState: "noSupport"
                });
            return;
        }
        
        switch (error.name) {
            case "NotFoundError":
                this.setState({
                    errorState: "noCamera"
                });
                break;
            case "NotAllowedError":
                this.setState({
                    errorState: "noPermission"
                });
                break;
            default:
                this.setState({
                    errorState: "genericError"
                });
                break;
        }
    }
    
    capture() {
        let picture = this.webcam.current.getScreenshot();
        WsController.sendBase64Picture(picture);
    }
}

export default CameraScreen;