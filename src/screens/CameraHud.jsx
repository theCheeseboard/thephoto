import React from 'react';

import Styles from './CameraHud.module.css';

class CameraHud extends React.Component {
    render() {
        return <div className={Styles.CameraHud}>
            <div className={Styles.CaptureButton} onClick={this.props.onCapture}>
                <svg width={100} height={100}>
                    <circle fill={"#00000000"} stroke={"#ffffff"} cx={50} cy={50} r={45} strokeWidth={5} />
                    <circle className={Styles.CaptureButtonInnerCircle} />
                </svg>
            </div>
        </div>
    }
}

export default CameraHud;