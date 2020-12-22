import React from 'react';

class CameraHud extends React.Component {
    render() {
        return <div className="cameraHud">
            <div className="captureButton" onClick={this.props.onCapture}/>
        </div>
    }
}

export default CameraHud;