import { Component } from '@angular/core';
import { Driver, Light, drivers } from './driver';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent {

  constructor() {
    this.checkLights();
  }

  checkLights() {
    drivers.forEach(d => {
      fetch("http://" + d?.ip + "/conf", {
        method: "GET",
      })
        .then(r => r.json())
        .then((conf: any) => {
          d.from = conf.from;
          d.to = conf.to;
          d.online = true;
          conf.lights.forEach((l: any, i: number) => {
            d.lights[i].auto = l.auto;
            d.lights[i].brightness = l.brightness;
            d.lights[i].on = l.on;
          });
        }, () => d.online = false)
        .catch(() => d.online = false);
    });

    setTimeout(() => this.checkLights(), 10000)
  }
}
