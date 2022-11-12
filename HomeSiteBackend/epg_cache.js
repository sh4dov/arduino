const http = require('http');

var now = new Date();

var job = (now, runNext) => {    
    var date = "" + now.getFullYear() + (now.getMonth() > 8 ? now.getMonth() + 1 : "0" + (now.getMonth() + 1)) + (now.getDate() > 9 ? now.getDate() : "0" + now.getDate()) + (now.getHours() > 9 ? now.getHours() : "0" + now.getHours());
    console.log("executing: " + date);
    http.get("http://localhost:3001/epg/" + date, res => {
        var data = "";
        res.on("data", chunk => data += chunk);
        res.on("end", () => {
            console.log("Executed: " + date);

            if(now.getHours() != 23 && runNext){
                now.setHours(now.getHours()+1);
                job(now, false);
            }
        });
    });
}

job(now, true);