import TransferController from './TransferController';

class WsController {
    #ws;
    #username;
    #wsStateChangeHandler;
    #connected;
    #userId;

    #pk;

    #symk;
    
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
        return new Promise(async (res, rej) => {
            //Retrieve the public key
            try {
                let pkData = await fetch(`${process.env.REACT_APP_RENDEZVOUS_SERVER_IS_SECURE === "true" ? "https://" : "http://"}${process.env.REACT_APP_RENDEZVOUS_SERVER}/keys/${room}`);

                let pkDataString = await pkData.text();
                const pemHeader = "-----BEGIN PRIVATE KEY-----";
                const pemFooter = "-----END PRIVATE KEY-----";
                let pemContents = pkDataString.substring(pemHeader.length, pkDataString.length - pemFooter.length);

                let binaryPk = window.atob(pemContents);

                let pkBuf = new ArrayBuffer(binaryPk.length);
                let bufView = new Uint8Array(pkBuf);
                for (let i = 0, strLen = binaryPk.length; i < strLen; i++) {
                    bufView[i] = binaryPk.charCodeAt(i);
                }

                this.#pk = await crypto.subtle.importKey("spki", pkBuf, {
                    name: "RSA-OAEP",
                    hash: "SHA-1"
                }, false, ["encrypt"]);
            } catch (err) {
                console.log(err);
                rej(err);
                return;
            }

            //Generate an AES symmetric key
            this.#symk = await crypto.subtle.generateKey({
                name: "AES-CBC",
                length: 256
            }, true, ["encrypt", "decrypt"]);

            let wsUrl = `${process.env.REACT_APP_RENDEZVOUS_SERVER_IS_SECURE === "true" ? "wss://" : "ws://"}${process.env.REACT_APP_RENDEZVOUS_SERVER}/rendezvous/${room}`
            this.#ws = new WebSocket(wsUrl);
            this.#ws.onopen = async (event) => {
                let exportedKey = await crypto.subtle.exportKey("raw", this.#symk);

                await this.sendMessage({
                    type: "connect",
                    username: username,
                    key: btoa(String.fromCharCode(...new Uint8Array(exportedKey)))
                }, (msg) => {
                    this.#connected = true;
                    this.#userId = msg.user;
                    this.#wsStateChangeHandler();
                    res();
                }, true);
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
        
        let dataView = new Uint8Array(data);
        if (String.fromCharCode(dataView[0]) === 'E') {
            let ivLen = dataView[1];
            let iv = dataView.slice(2, 2 + ivLen);
            let encryptedData = dataView.slice(2 + ivLen);

            data = await crypto.subtle.decrypt({
                name: "AES-CBC",
                iv: iv
            }, this.#symk, encryptedData);
        }

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
    
    async sendMessage(message, replyHandler, encryptWithPk) {
        message.seq = this.#seq++;
        
        if (this.#userId !== null) {
            message.userId = this.#userId;
        }
        
        let textEncoder = new TextEncoder();
        let payload = textEncoder.encode(JSON.stringify(message));
        
        if (encryptWithPk) {
            try {
                payload = await crypto.subtle.encrypt({
                    name: "RSA-OAEP"
                }, this.#pk, payload);
            } catch (err) {
                console.log(err);
                return;
            }
        } else {
            try {
                let iv = crypto.getRandomValues(new Uint8Array(16));
                let payloadData = await crypto.subtle.encrypt({
                    name: "AES-CBC",
                    iv: iv
                }, this.#symk, payload);

                payload = new Uint8Array(2 + iv.length + payloadData.byteLength);
                payload.set(['E'.charCodeAt(0), iv.length]);
                payload.set(iv, 2);
                payload.set(new Uint8Array(payloadData), iv.length + 2);
            } catch (err) {
                console.log(err);
                return;
            }
        }
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
        
        //Send the picture a few chunks at a time
        let sendChunk = (replySeq) => {
            let pictureData = picture.substr(job.progress(), 131072);
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
                    if (data.continue) sendChunk(data.seq);
                }
            });
            job.addProgress(pictureData.length);
        };
        
        sendChunk();
    }
}

let controller = new WsController();

export default controller;