import React from 'react';

import Styles from './CameraHud.module.css';

class CameraHud extends React.Component {
    render() {
        return <div className={Styles.CameraHud}>
            <div className="captureButton" onClick={this.props.onCapture}/>
        </div>
    }
}

export default CameraHud;