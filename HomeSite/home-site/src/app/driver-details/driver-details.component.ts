import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { HttpClientModule, JsonpClientBackend } from '@angular/common/http';
import { Driver, Light, drivers } from '../driver';

@Component({
  selector: 'app-driver-details',
  templateUrl: './driver-details.component.html',
  styleUrls: ['./driver-details.component.css']
})
export class DriverDetailsComponent implements OnInit {
  driver: Driver | undefined;

  constructor(private route: ActivatedRoute,
    private http: HttpClientModule) { }

  ngOnInit(): void {
    const routePrams = this.route.snapshot.paramMap;
    const id = Number(routePrams.get('id'));

    if (id >= 0 && id < drivers.length) {
      this.driver = drivers[id];
    }
  }

  changeState(light: Light) {
    fetch("http://" + this.driver?.ip + "/onoff", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id: light.id, value: !light.on })
    }).then(() => light.on = !light.on);
  }

  changeAuto(light: Light) {
    fetch("http://" + this.driver?.ip + "/auto", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id: light.id, value: light.auto })
    });
  }

  changeBrightness(light: Light) {
    fetch("http://" + this.driver?.ip + "/brightness", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id: light.id, value: light.brightness })
    });
  }

  saveSettings() {
    fetch("http://" + this.driver?.ip + "/save", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ from: this.driver?.from, to: this.driver?.to })
    });
  }
}
