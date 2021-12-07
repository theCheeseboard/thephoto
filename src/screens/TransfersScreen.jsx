import React from 'react';
import TransferController from "../TransferController";
import TransferItem from "./TransferItem";

import Styles from "./TransfersScreen.module.css";

class TransfersScreen extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            jobs: []
        };
    }

    componentDidMount() {
        TransferController.on("progressUpdated", this.progressUpdated.bind(this));

        this.progressUpdated();
    }

    componentWillUnmount() {
        TransferController.removeListener("progressUpdated", this.progressUpdated.bind(this));
    }

    progressUpdated() {
        this.setState({
            jobs: [...TransferController.jobs()].reverse()
        });
    }

    render() {
        return <div className={Styles.TransferGrid}>
            {this.state.jobs.map(job => <TransferItem job={job} />)}
        </div>
    }
}

export default TransfersScreen;