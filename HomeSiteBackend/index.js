const express = require('express')
const http = require('http');

const app = express();

const sockets = ["192.168.100.51", "192.168.100.53"];

app.get('/', (request, response) => {
    response.send("Home Site backend");    
});

app.get("/api/params", (request, response) => {
    var data = ""
    http.get("http://192.168.100.49/params", res => {
        res.on("data", chunk => data += chunk);
        res.on("error", err => console.log(err));
        res.on("end", () => {
            const params = data.split(' ');
            const result = {
                pv: {
                    voltage: params[13],
                    amp: params[12],
                    watt: params[19]
                },
                acu : {
                    voltage: params[8],
                    discharge: params[15],
                    charging: params[9],
                    soc: params[10]
                },
                load: params[6],
                ac: {
                    voltage: params[0],
                    hz: params[1]
                },
                ac_out: {
                    voltage: params[2],
                    hz: params[3]
                },
                power: {
                    apparent: params[4],
                    active: params[5]
                },
                temp: params[11],
                v_bus: params[7]
            }
            response.send(JSON.stringify(result));
        });
    });
});

app.get("/api/stats", (request, response) =>{
    var data = ""
    http.get("http://192.168.100.49/stats", res => {
        res.on("data", chunk => data += chunk);
        res.on("error", err => console.log(err));
        res.on("end", () => {
            var result = {};
            const arr = data.split(".");
            result.year = +arr[0].substring(0, 8);
            result.month = +arr[1].substring(0, 8);
            result.day = +arr[2].substring(0, 8);
            result.total = +arr[3].substring(0, 8);

            if(!result.day) {
                const d = new Date();
                data = "";
                let q = d.getFullYear() + (d.getMonth() >= 9 ? "" + (d.getMonth() + 1) : "0" + (d.getMonth() + 1)) + (d.getDate() > 9 ? "" + d.getDate() : "0" + d.getDate());
                http.get("http://192.168.100.49/qed?d=" + q, res => {
                    res.on("data", chunk => data += chunk);
                    res.on("end", () => {
                        result.day = +data.substring(1, 9);

                        response.send(JSON.stringify(result));
                    });
                });
            } else {
                response.send(JSON.stringify(result));
            }
        });
    });
});

app.get("/api/sockets", (request, response) => {    
    const result = [null, null];

    sockets.forEach((ip, id) => {
        http.get("http://" + ip, res => {
            let data = "";
            res.on("data", chunk => data += chunk);
            res.on("error", err => console.log(err));
            res.on("end", () => {
                let socket = {
                    id: id,
                    data: data
                };

                result[id] = socket;

                if(result.every(r => !!r)){
                    response.json(result);
                }
            });
        });
    });
});

app.get("/api/sockets/:id/on", (request, response) => {
    const id = +request.params["id"];

    if((!id && id != 0) || id < 0 || id > sockets.length - 1){
        response.status(500).send("unknown id");
        return;
    }

    http.get("http://" + sockets[id] + "/on", res => {
        let data = "";
        res.on("data", chunk => data += chunk);
        res.on("error", err => console.log(err));
        res.on("end", () => {
            response.send(data);
        });
    });
});

app.get("/api/sockets/:id/off", (request, response) => {
    const id = +request.params["id"];

    if((!id && id != 0) || id < 0 || id > sockets.length - 1){
        response.status(500).send("unknown id");
        return;
    }

    http.get("http://" + sockets[id] + "/off", res => {
        let data = "";
        res.on("data", chunk => data += chunk);
        res.on("error", err => console.log(err));
        res.on("end", () => {
            response.send(data);
        });
    });
});

app.get("/api/worktype", (request, response) => {
    http.get("http://192.168.100.49/worktype", res => {
        let data = "";
        res.on("data", chunk => data += chunk);
        res.on("error", err => console.log(err));
        res.on("end", () => {
            response.send(data);
        });
    });
});

app.get("/api/sub", (request, response) => {
    http.get("http://192.168.100.49/sub", res => {
        let data = "";
        res.on("data", chunk => data += chunk);
        res.on("error", err => console.log(err));
        res.on("end", () => {
            response.send(data);
        });
    });
});

app.get("/api/sbu", (request, response) => {
    http.get("http://192.168.100.49/sbu", res => {
        let data = "";
        res.on("data", chunk => data += chunk);
        res.on("error", err => console.log(err));
        res.on("end", () => {
            response.send(data);
        });
    });
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
