const XmlReader = require("xml-reader");
const fs = require("fs");
const express = require('express');

const app = express();

const channels = [
    {name: "Discovery Channel", no: 128},
    {name: "TVN Turbo", no: 212},
    {name: "SCI FI", no: 108}, 
    {name: "AXN", no: 114}, 
    {name: "AMC", no: 115}, 
    {name: "Comedy Central", no: 103}, 
    {name: "Cinemax", no: 78}, 
    {name: "Cinemax 2", no: 79}, 
    {name: "Canal+ Film", no: 73}, 
    {name: "Ale Kino+", no: 95},
    {name: "Discovery Science", no: 139}, 
    {name: "Discovery Life", no: 231}, 
    {name: "FilmBox Action", no: 82}, 
    {name: "Filmbox Arthouse", no: 84}, 
    {name: "Filmbox Extra HD", no: 81}, 
    {name: "Filmbox Family", no: 83},
    {name: "Filmbox Premium HD", no: 80}, 
    {name: "AXN Black", no: 291}, 
    {name: "AXN Spin", no: 293}, 
    {name: "AXN White", no: 292}, 
    {name: "Canal+ Dokument", no: 126}, 
    {name: "Canal+ 1", no: 72}, 
    {name: "FOX", no: 113},
    {name: "HBO", no: 75}, 
    {name: "HBO 2", no: 76}, 
    {name: "HBO 3", no: 77}, 
    {name: "Kino Polska", no: 43}, 
    {name: "METRO", no: 56}, 
    {name: "Paramount Channel", no: 104}, 
    {name: "Polsat 2", no: 10}, 
    {name: "Polsat Comedy Central Extra", no: 109},
    {name: "Polsat Film", no: 120}, 
    {name: "Kino TV", no: 44}, 
    {name: "Polsat Games", no: 192}, 
    {name: "Polsat", no: 7}, 
    {name: "Stopklatka TV", no: 21}, 
    {name: "Super Polsat", no: 67}, 
    {name: "Tele 5", no: 289}, 
    {name: "TLC", no: 222}, 
    {name: "TTV", no: 20}, 
    {name: "Puls 2", no: 24},
    {name: "TV Puls", no: 23}, 
    {name: "TV 4", no: 8}, 
    {name: "TV 6", no: 68}, 
    {name: "TVN 7", no: 9}, 
    {name: "TVN Fabuła", no: 111}, 
    {name: "TVN", no: 5}, 
    {name: "TVN Style", no: 213}, 
    {name: "TVP 1", no: 11}, 
    {name: "TVP 2", no: 12}, 
    {name: "TVP HD", no: 13}, 
    {name: "TVP Kultura", no: 16},
    {name: "Warner TV", no: 106}, 
    {name: "WP", no: 69}, 
    {name: "ZOOM TV", no: 22}];     
    
const cache = {};

const getDate = (s) => {
    return utcToLocal(new Date(+s.substring(0,4), +s.substring(4, 6) - 1, +s.substring(6, 8), +s.substring(8, 10), s.substring(10,12), 0, 0));
};

const utcToLocal = (date) => {
    var newDate = new Date(date.getTime()+date.getTimezoneOffset()*60*1000);

    var offset = date.getTimezoneOffset() / 60;
    var hours = date.getHours();

    newDate.setHours(hours - offset);

    return newDate;   
}

app.get("/epg/:date", (request, response) =>{
    const date = request.params["date"];
    if(date.length != 10){
        response.status(500).send("invalid date");
        return;
    }

    Object.keys(cache).forEach(k => {
        const now = new Date();
        const cd = new Date(+k.substring(0,4), +k.substring(4,6) - 1, +k.substring(6,8), +k.substring(8,10), 59, 59, 0);
        if(cd < now){
            delete cache[k];
        }
    });

    if(cache[date]){
        response.json(cache[date]);        
        return;
    }
    
    const from = new Date(+date.substring(0,4), +date.substring(4,6) - 1, +date.substring(6,8), +date.substring(8,10), 0, 0, 0);
    const to = new Date(from.getFullYear(), from.getMonth(), from.getDate(), from.getHours() + 1, 0, 0, 0);
    const result = [];
    const channelsIco = [];

    const reader = XmlReader.create({stream: true});

    reader.on("tag:channel", data => {
        if(!data){
            return;
        }

        if(channels.some(c => c.name == data.attributes.id)){            
            const icon = data.children.filter(c => c.name == "icon");
            const result = {
                id: data.attributes.id,
                icon: icon.length ? icon[0].attributes.src : ""
            }

            if(result.icon){
                channelsIco.push(result);
            }
        }
    });

    reader.on('tag:programme', data => {
        if(channels.some(c => c.name == data.attributes.channel) && getDate(data.attributes.start) >= from && getDate(data.attributes.start) < to){
            const img = data.children.filter(c => c.name == "icon");
            const desc = data.children.filter(c => c.name == "desc");
            const title = data.children.filter(c => c.name == "title");
            const no = channels.filter(c => c.name == data.attributes.channel);

            const item = {
                channel: data.attributes.channel,
                start: getDate(data.attributes.start),
                stop: getDate(data.attributes.stop),
                title: title.length ? title[0].children[0].value : "",
                categories: data.children.filter(c => c.name == "category").map(c => c.children[0].value),
                desc: desc.length ? desc[0].children[0].value : "",
                img: img.length ? img[0].attributes.src : "",
                no: no.length ? no[0].no : ""
            };
            
            if(!item.categories.some(c => c == "Kodi W Pigulce")){
                result.push(item);
            }
        }
    });

    reader.on("done", () => {
        result.forEach(r => {
            const icons = channelsIco.filter(i => i.id == r.channel);
            r.channelImg = icons.length ? icons[0].icon : ""
        });

        cache[date] = result;
        response.json(result);
    });

    //const s = fs.readFile("/home/orangepi/homesitebackend/pl.xml", "utf8", (err, data) => {
    const s = fs.readFile("C:\\Users\\z8qar\\Downloads\\pl.xml", "utf8", (err, data) => {
        reader.parse(data);
    });
});

const PORT = 3001;
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});