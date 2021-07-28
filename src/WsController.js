import TransferController from './TransferController';

class WsController {
    #ws;
    #username;
    #wsStateChangeHandler;
    #connected;
    #userId;
    
    #seq;
    #replies;
    
    constructor() {
        this.#ws = null;
        this.#seq = 0;
        this.#replies = {};
    }
    
    setWsStateChangeHandler(handler) {
        this.#wsStateChangeHandler = handler;
    }
    
    connect(room, username) {
        return new Promise((res, rej) => {
            // let wsUrl = "ws://" + room + ":26158/";
            let wsUrl = `${process.env.REACT_APP_RENDEZVOUS_SERVER}/rendezvous/${room}`
            this.#ws = new WebSocket(wsUrl);
            this.#ws.onopen = (event) => {
                this.sendMessage({
                    type: "connect",
                    username: username
                }, (msg) => {
                    this.#connected = true;
                    this.#userId = msg.user;
                    this.#wsStateChangeHandler();
                    res();
                });
            };
            this.#ws.onerror = (event) => {
                this.#ws = null;
                this.#connected = false;
                
                this.#wsStateChangeHandler();
                rej();
            }
            this.#ws.onclose = (event) => {
                this.#ws = null;
                this.#connected = false;
                this.#userId = null;
                this.#wsStateChangeHandler();
            }
            this.#ws.onmessage = this.wsMessage.bind(this);
        });
    }
    
    async wsMessage(event) {
        let data;
        if (event.data.arrayBuffer) {
            data = await event.data.arrayBuffer();
        } else {
            data = await new Response(event.data).arrayBuffer();
        }
        
        //TODO: decrypt data
        
        let decoder = new TextDecoder("utf-8");
        let msg = JSON.parse(decoder.decode(data));

        if (msg.type === "serverKeepalive") return; //Keepalive message
        
        if (msg.replyTo !== null) {
            this.#replies[msg.replyTo](msg);
        }
    }
    
    isConnected() {
        return this.#connected;
    }
    
    sendMessage(message, replyHandler) {
        message.seq = this.#seq++;
        
        if (this.#userId !== null) {
            message.userId = this.#userId;
        }
        
        let textEncoder = new TextEncoder();
        let payload = textEncoder.encode(JSON.stringify(message));
        
        //TODO: encrypt payload
        
        this.#ws.send(payload);
        
        if (replyHandler !== null) {
            this.#replies[message.seq] = replyHandler;
        }
        
        return message.seq;
    }
    
    disconnect() {
        if (this.isConnected()) {
            this.sendMessage({
                type: "disconnect",
                userId: this.#userId
            }, () => this.#ws.close.bind(this));
        }
        this.#ws.close();
    }
    
    sendBase64Picture(picture) {
        let job = TransferController.makeTransferJob();
        job.setImageData(picture);
        
        let mimeType = picture.substring(5, picture.indexOf(";"));
        
        picture = picture.substr(picture.indexOf("base64,") + 7);
        job.setTotal(picture.length);
        
        //Send the picture 4096 chunks at a time
        let sendChunk = (replySeq) => {
            let pictureData = picture.substr(job.progress(), 4096);
            let message = {
                type: "picture",
                length: picture.length,
                mimeType: mimeType,
                data: pictureData
            };
            
            if (replySeq !== null) {
                message.replyTo = replySeq;
            }
            
            this.sendMessage(message, (data) => {
                if (job.progress() >= job.total()) {
                    //We're done here
                    //TODO: change the state of the transfer
                } else {
                    if (data.continue) sendChunk(data.replyTo);
                }
            });
            job.addProgress(pictureData.length);
        };
        
        sendChunk();
    }
}

let controller = new WsController();

export default controller;