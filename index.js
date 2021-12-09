const Crypto = require('crypto');
const express = require('express');
const { resolveNaptr } = require('dns');
let app = express();

const expressWs = require('express-ws')(app);

let passwords = {};
let serverCount = {};
let currentServer = {};
let keys = {};

app.use(express.text({
    type: [
        "application/x-pem-file"
    ]
}));

app.get("/setup", (req, res) => {
    //Set up a rendezvous server
    let serverNumber = Math.floor(Math.random() * 10000);
    while (serverCount[serverNumber]) {
        serverNumber = Math.floor(Math.random() * 10000);
    }

    console.log(`Rendezvous server set up for ID ${serverNumber}`);

    let password = Crypto.randomBytes(32).toString('base64').slice(0, 32);
    passwords[serverNumber] = password;
    serverCount[serverNumber] = 0;
    res.send({
        serverNumber: serverNumber,
        token: password
    });
});
app.use("/setup/:id", (req, res, next) => {
    console.log(`Installing Rendezvous client for ${req.params.id}`);

    if (req.method !== "GET") {
        console.log("FAIL: Method not GET");
        res.send(405);
        return;
    }

    if (!passwords[req.params.id]) {
        console.log("FAIL: Client not set up");
        res.send(403);
        return;
    }

    let auth = req.header("Authorization");
    if (!auth || !auth.startsWith("Bearer ") || passwords[req.params.id] !== auth.substr(7)) {
        console.log("FAIL: Token invalid");
        res.send(403);
        return;
    }

    if (currentServer[req.params.id]) {
        console.log("FAIL: Server already set up");
        res.send(400);
        return;
    }

    next();
})
app.ws("/setup/:id", (ws, req) => {
    serverCount[req.params.id]++;
    currentServer[req.params.id] = ws;

    console.log(`New rendezvous server for ${req.params.id}`);

    //Keep the client alive
    let timeout = setInterval(() => {
        ws.send(JSON.stringify({
            seq: -1,
            type: "serverKeepalive"
        }));
    }, 10000);

    ws.on("close", () => {
        clearInterval(timeout);
        if (currentServer[req.params.id] === ws) delete currentServer[req.params.id];
        serverCount[req.params.id]--;
    });
});


app.post("/keys/:id", (req, res) => {
    if (!passwords[req.params.id]) {
        console.log("FAIL: Client not set up");
        res.send(403);
        return;
    }

    let auth = req.header("Authorization");
    if (!auth || !auth.startsWith("Bearer ") || passwords[req.params.id] !== auth.substr(7)) {
        console.log("FAIL: Token invalid");
        res.send(403);
        return;
    }

    if (!req.body) {
        console.log("FAIL: Body invalid");
        res.send(400);
        return;
    }

    keys[req.params.id] = req.body.toString();
    res.sendStatus(204);
});
app.get("/keys/:id", (req, res) => {
    if (!keys[req.params.id]) {
        res.sendStatus(404);
        res.end();
    }

    res.setHeader("Access-Control-Allow-Origin", "*");
    res.send(keys[req.params.id]);
});

app.use("/rendezvous/:id", (req, res, next) => {
    if (req.method !== "GET") {
        res.send(405);
        res.end();
        return;
    }

    if (!currentServer[req.params.id]) {
        res.send(500);
        res.end();
        return;
    }

    next();
})
app.ws("/rendezvous/:id", (ws, req) => {
    let server = currentServer[req.params.id];
    delete currentServer[req.params.id];

    console.log(`New rendezvous client for ${req.params.id}`);

    server.send(JSON.stringify({
        type: "rendezvousClientConnect"
    }));

    //Keep the client alive
    let timeout = setInterval(() => {
        try {
            ws.send(JSON.stringify({
                seq: -1,
                type: "serverKeepalive"
            }));
        } catch {
            //Ignore
        }
    }, 10000);

    ws.on("message", msg => {
        if (server.readyState === 1) server.send(msg)
    });
    server.on("message", msg => {
        if (ws.readyState === 1) ws.send(msg)
    });
    ws.on("close", () => {
        if (server.readyState === 1) {
            clearTimeout(timeout);
            server.close()
        }
    });
    server.on("close", () => {
        if (ws.readyState === 1) ws.close()
    });
});

app.use(express.static("../thephoto-react/build/"));

let port = process.env.PORT || 4000;
app.listen(port, () => {
    console.log(`Listening on port ${port}`)
});