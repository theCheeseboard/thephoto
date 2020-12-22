import React from 'react';

class Header extends React.Component {
    render() {
        return <div className="header">
            <div className="systemButton menu" />
            <div className={this.className("camera")} onClick={() => this.changePage("camera")}>Camera</div>
            <div className={this.className("transfers")} onClick={() => this.changePage("transfers")}>Transfers</div>
            <div className={this.className("upload")} onClick={() => this.changePage("upload")}>Upload</div>
            <div className="spacer" />
            <div className="systemButton close" onClick={this.props.onExit} />
        </div>;
    }
    
    changePage(page) {
        this.props.onChangePage(page);
    }
    
    className(button) {
        let classes = ["headerButton", button];
        if (this.props.page === button) classes.push("selected");
        return classes.join(" ");
    }
}

export default Header;