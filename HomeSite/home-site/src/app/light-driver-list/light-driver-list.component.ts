import { HttpClient } from '@angular/common/http';
import { Component, OnInit, Input } from '@angular/core';
import { Driver, drivers } from '../driver';
import { ESBData } from '../ESBData';

@Component({
  selector: 'app-light-driver-list',
  templateUrl: './light-driver-list.component.html',
  styleUrls: ['./light-driver-list.component.css']
})
export class LightDriverListComponent implements OnInit {
  drivers: Driver[] = drivers;
  esb: ESBData = {
    day:"",
    month:"",
    year:"",
    total:"",
    params: []
  };
  sockets = ["", ""];
  ips = ["192.168.100.51", "192.168.100.53"];

  constructor(private http: HttpClient) { }

  ngOnInit(): void {
    this.getESBData();
    this.getSockets();
  }

  getESBData() {
    let day = 0;
    fetch("http://192.168.100.49/stats", {
      method: "GET"
    }).then(r => r.text())
    .then(data => {
      const arr = data.split(".");
      const year = +arr[0].substring(0, 8);
      const month = +arr[1].substring(0, 8);
      day = +arr[2].substring(0, 8);
      const total = +arr[3].substring(0, 8);
      this.esb.year = year > 1000 ? year / 1000 + " kW" : year + " W";
      this.esb.month = month > 1000 ? month / 1000 + " kW" : month + " W";
      if(day){
      this.esb.day = day > 1000 ? day / 1000 + " kW" : day + " W";
      }
      this.esb.total = total > 1000 ? total / 1000 + " kW" : total + " W";
    })
    .then(() => fetch("http://192.168.100.49/params"))
    .then(r => r.text())
    .then(data => {
      this.esb.params = data.split(' ');
    })
    .then(() => {
      if(!day){
        const d = new Date();
        let q = d.getFullYear() + (d.getMonth() >= 9 ? "" + (d.getMonth() + 1) : "0" + (d.getMonth() + 1)) + (d.getDate() >= 9 ? "" + d.getDate() : "0" + d.getDate());
        return fetch("http://192.168.100.49/qed?d=" + q, {
          method: "GET"
        }).then(r => r.text())
        .then(data => {
          const day = +data.substring(1, 9);
          this.esb.day = day > 1000 ? day / 1000 + " kW" : day + " W";
        })
      }

      return null;
    })
    .finally(() => {
      setTimeout(() => this.getESBData(), 10000);
    });
  }

  getSockets(){
    this.ips.forEach((ip, id, arr) => this.getSocket(ip, id));
  }

  getSocket(ip: string, id: number){
    fetch("http://" + ip, {
      method: "GET"
    }).then(r => r.text())
    .then(data => {
      this.sockets[id] = data;
    })
    .finally(() => {
      setTimeout(() => this.getSocket(ip, id), 10000);
    });
  }

  turnOn(i: number){
    fetch("http://" + this.ips[i] + "/on", {
      method: "GET"
    })
  }

  turnOff(i: number){
    fetch("http://" + this.ips[i] + "/off", {
      method: "GET"
    })
  }
}
