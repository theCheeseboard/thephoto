import React from "react";

import Styles from "./TransferItem.module.css";

class TransferItem extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            percent: 1
        }
    }

    componentDidMount() {
        setInterval(() => {
            this.setState(state => ({
                percent: state.percent > 1.5 ? 0 : state.percent + 0.01
            }));
        }, 10);
    }

    polarToCartesian(centerX, centerY, radius, angleInDegrees) {
        let angleInRadians = (angleInDegrees-90) * Math.PI / 180.0;

        return {
            x: centerX + (radius * Math.cos(angleInRadians)),
            y: centerY + (radius * Math.sin(angleInRadians))
        };
    }

    describeArc(x, y, radius, startAngle, endAngle){
        let start = this.polarToCartesian(x, y, radius, endAngle);
        let end = this.polarToCartesian(x, y, radius, startAngle);

        let arcSweep = endAngle - startAngle <= 180 ? "0" : "1";

        let d = [
            "M", start.x, start.y,
            "A", radius, radius, 0, arcSweep, 0, end.x, end.y,
            // "L", x,y,
            // "L", start.x, start.y
        ].join(" ");

        return d;
    }

    renderProgressPath() {
        let percent = this.props.job.percentComplete();
        if (percent >= 1) {
            return <>
                <circle id={"circle"} stroke={"#ffffff"} fill={"#00000000"} strokeWidth={10} cx={55} cy={55} r={50} />
                <path id={"check"} stroke={"#ffffff"} fill={"#00000000"} strokeWidth={10} d="M 30 60 L 50 75 L 75 30" />
            </>
        } else {
            return <path id={"arc"} stroke={"#ffffff"} fill={"#00000000"} strokeWidth={10} d={this.describeArc(55, 55, 50, 0, percent * 360)} />
        }
    }

    render() {
        return <div className={Styles.TransferItem} style={{
            backgroundImage: `linear-gradient( rgba(0, 0, 0, 0.5), rgba(0, 0, 0, 0.5) ), url("${this.props.job.imageData()}")`
        }}>
            <svg className={Styles.Spinner} width={110} height={110}>
                {/*<circle cx={50} cy={50} r={50} />*/}
                {this.renderProgressPath()}
            </svg>
        </div>
    }
}

export default TransferItem;