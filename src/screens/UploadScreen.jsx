import React from 'react';

import Styles from './UploadScreen.module.css';

import WsController from '../WsController'

class UploadScreen extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            isDropping: false
        };

        this.filePicker = React.createRef();
    }

    dragEnter(event) {
        let types = event.dataTransfer.types;
        if (!types.includes("Files")) return;

        event.preventDefault();
        event.stopPropagation();

        this.setState({
            isDropping: true
        });
    }

    dragLeave(event) {
        event.preventDefault();
        event.stopPropagation();

        this.setState({
            isDropping: false
        });
    }

    dragOver(event) {
        event.preventDefault();
        event.stopPropagation();
    }

    drop(event) {
        event.preventDefault();
        event.stopPropagation();

        this.setState({
            isDropping: false
        });

        let files;
        if (event.dataTransfer.items) {
            files = Array.from(event.dataTransfer.items).filter(item => item.kind === "file").map(item => item.getAsFile());
        } else {
            files = event.dataTransfer.files;
        }

        this.uploadFiles(files);
    }

    pickFileClicked() {
        this.filePicker.current.click()
    }

    pickFile(event) {
        this.uploadFiles(Array.from(event.target.files));
        event.target.value = "";
    }

    uploadFiles(files) {
        try {
            const initialLength = files.length;
            files = files.filter(file => ["image/png", "image/jpeg", "image/gif"].includes(file.type));
            if (files.length === 0) throw new Error("Not a file!");

            if (files.length !== initialLength) {
                //Let the user know that some files will not be uploaded
            }
    
            console.log(`Uploading ${files.length} files.`)
            for (let file of files) {
                let reader = new FileReader();
                reader.readAsDataURL(file);
                reader.onloadend = () => {
                    WsController.sendBase64Picture(reader.result);
                };
            }

            this.props.onUploadSelected();
        } catch (err) {
            console.log("Not valid files");
        }
    }

    render() {
        return <div className={[Styles.UploadScreen, this.state.isDropping ? Styles.Dropping : ""].filter(cls => cls !== "").join(" ")}
            onDragEnter={this.dragEnter.bind(this)}
            onDragLeave={this.dragLeave.bind(this)}
            onDragOver={this.dragOver.bind(this)}
            onDrop={this.drop.bind(this)} >
            <div className={Styles.UploadContainer}>
                <h1>Upload a picture</h1>
                Go ahead and drop a picture here to upload it.
                <div className={Styles.UploadButton} onClick={this.pickFileClicked.bind(this)}>Pick File</div>
                <input type="file" multiple={true} onChange={this.pickFile.bind(this)} className={Styles.FilePicker} ref={this.filePicker} />
            </div>
        </div>
    }
}

export default UploadScreen;