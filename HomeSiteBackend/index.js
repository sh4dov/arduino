const express = require('express')
const http = require('http');

const app = express();

const sockets = ["192.168.100.51", "192.168.100.53"];

const drivers = [
    {
        id: 1,
        name: "Kuchnia dół",
        ip: "192.168.100.32",
        from: 7,
        to: 16,
        online: false,
        error: "",
        lights: [
            {
                name: "Oswietlenie górne",
                id: 1,
                auto: true,
                on: true,
                brightness: 100
            },
            {
                name: "Oswietlenie szafek",
                id: 2,
                auto: false,
                on: false,
                brightness: 100
            },
            {
                name: "Oswietlenie blat",
                id: 3,
                auto: false,
                on: false,
                brightness: 100
            },
            {
                name: "Oswietlenie podłoga",
                id: 4,
                auto: true,
                on: false,
                brightness: 100
            }
        ]
    },
    {
        id: 2,
        name: "Korytarz dół",
        ip: "192.168.100.33",
        from: 7,
        to: 16,
        online: false,
        error: "",
        lights: [
            {
                name: "Plafon",
                id: 1,
                auto: true,
                on: false,
                brightness: 100
            },
            {
                name: "Główne",
                id: 2,
                auto: true,
                on: false,
                brightness: 100
            }
        ]
    },
    {
        id: 3,
        name: "Korytarz góra",
        ip: "192.168.100.34",
        from: 7,
        to: 16,
        online: false,
        error: "",
        lights: [
            {
                name: "Plafon",
                id: 1,
                auto: true,
                on: false,
                brightness: 50
            },
            {
                name: "Główne",
                id: 2,
                auto: true,
                on: false,
                brightness: 50
            }
        ]
    }
];

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

app.get("/api/qem/:d", (request, response) => {
    const d = +request.params["d"];
    var data = "";

    http.get("http://192.168.100.49/qem?d=" + d, res => {
        res.on("data", chunk => data += chunk);
        res.on("end", () => {
            const result = {value: +data.substring(1, 9)};

            response.send(JSON.stringify(result));
        });
    });
});

app.get("/api/qed/:d", (request, response) => {
    const d = +request.params["d"];
    var data = "";

    http.get("http://192.168.100.49/qed?d=" + d, res => {
        res.on("data", chunk => data += chunk);
        res.on("end", () => {
            const result = {value: +data.substring(1, 9)};

            response.send(JSON.stringify(result));
        });
    });
});

app.get("/api/stats", (request, response) =>{
    var data = "";
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

app.get("/api/drivers", (request, response) => {
    var result = [];
    drivers.forEach((d, id) => {
        result.push(null);

        http.get("http://" + d.ip + "/conf", res => {
            let data = "";
            res.on("data", chunk => data += chunk);
            res.on("error", err => {
                d.error = err;
                d.online = false;
                result[id] = d;

                if(result.every(r => !!r)){
                    response.json(result);
                }
            });
            res.on("end", () => {
                const conf = JSON.parse(data);
                d.from = conf.from;
                d.to = conf.to;
                d.online = true;
                conf.lights.forEach((l, i) => {
                    d.lights[i].auto = l.auto;
                    d.lights[i].brightness = l.brightness;
                    d.lights[i].on = l.on;
                });

                result[id] = d;
                if(result.every(r => !!r)){
                    response.json(result);
                }
            });
        });
    });
});

app.get("/api/drivers/:id", (request, response) => {
    const id = +request.params["id"];
    const dr = drivers.filter(d => d.id == id);
    const d = dr.length ? dr[0] : null;

    if(!d){
        response.status(500).send("unknown id");
        return;
    }

    http.get("http://" + d.ip + "/conf", res => {
        let data = "";
        res.on("data", chunk => data += chunk);
        res.on("error", err => {
            d.error = err;
            d.online = false;

            response.json(d);
        });
        res.on("end", () => {
            const conf = JSON.parse(data);
            d.from = conf.from;
            d.to = conf.to;
            d.online = true;
            conf.lights.forEach((l, i) => {
                d.lights[i].auto = l.auto;
                d.lights[i].brightness = l.brightness;
                d.lights[i].on = l.on;
            });

            response.json(d);
        });
    });
});

app.post("/api/drivers/:id/onoff", (request, response) => {
    const id = +request.params["id"];
    const dr = drivers.filter(d => d.id == id);
    const driver = dr.length ? dr[0] : null;

    if(!driver){
        response.status(500).send("unknown id");
        return;
    }

    let data = "";
    request.on("data", chunk => data += chunk);
    request.on("end", () => {
        const req = http.request({
            hostname: driver.ip,
            path: "/onoff",
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Content-Lenght": data.length
            }
        }, res => {
            data = "";
            res.on("data", chunk => data += chunk);
            res.on("error", err => {
                response.status(500).send(err);
            });
            res.on("end", () => {
                response.send(data || "ok");
            });
        });
        req.write(data);
        req.end();
    });
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
