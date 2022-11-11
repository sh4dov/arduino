import { HttpClient } from '@angular/common/http';
import { Component, OnDestroy, OnInit } from '@angular/core';
import { finalize, switchMap, tap } from 'rxjs';
import { RepeateService } from '../services/RepeateService';

interface Stats {
  year: number,
  month: number,
  day: number,
  total: number
}

interface StatsDisplay {
  year: string,
  month: string,
  day: string,
  total: string
}

interface Params {
  pv: {
    voltage: string,
    amp: string,
    watt: string
  },
  acu: {
    voltage: string,
    discharge: string,
    charging: string,
    soc: string
  },
  load: string,
  ac: {
    voltage: string,
    hz: string
  },
  ac_out: {
    voltage: string,
    hz: string
  },
  power: {
    apparent: string,
    active: string
  },
  temp: string,
  v_bus: string
}

@Component({
  selector: 'app-esb',
  templateUrl: './esb.component.html',
  styleUrls: ['./esb.component.css']
})
export class ESBComponent implements OnInit, OnDestroy {
  public stats: StatsDisplay = {day: "", month: "", total: "", year: ""};
  public params: Params = {ac: {voltage: "", hz: ""}, ac_out: {voltage: "", hz: ""}, acu: {charging: "", discharge: "", soc: "", voltage: ""},load: "", power: {active: "", apparent: ""}, pv: {amp: "", voltage: "", watt: ""}, temp: "", v_bus: ""};
  public workType = "";

  constructor(
    private http: HttpClient,
    private repeate: RepeateService) { }

  ngOnDestroy(): void {
    this.repeate.unsubscribe();
  }

  ngOnInit(): void {
    this.repeate.subscribe(() => this.getData());
  }

  getData(){
    this.getStats()
    .pipe(
      switchMap(() => this.getParams()),
      switchMap(() => this.getWorkType()),
      finalize(() => this.repeate.repeate()))
    .subscribe();
  }

  getStats(){
    return this.http.get<Stats>("/api/stats")
    .pipe(tap(stats => {
      this.stats = {
        day: this.getStatsString(stats.day),
        month: this.getStatsString(stats.month),
        total: this.getStatsString(stats.total),
        year: this.getStatsString(stats.year)
      };
    }))
  }

  getParams(){
    return this.http.get<Params>("/api/params")
    .pipe(tap(params => {
      this.params = params;
    }))
  }

  getWorkType(){
    return this.http.get("/api/worktype", {responseType: "text"})
    .pipe(
      tap(worktype => {
      if(worktype == "sub"){
        this.workType = "SUB (PV AC ACU)";
      }else if(worktype == "sbu"){
        this.workType = "SBU (PV ACU AC)";
      }
      else{
        this.workType = "Unknown";
      }
    }));
  }

  getStatsString(value: number): string {
    return value > 1000 ? value / 1000 + " kW" : value + " W";
  }

  setSub() {
    this.http.get("/api/sub", {responseType: "text"})
    .pipe(tap(response =>{
      if(response.indexOf("ACK") >= 0){
        this.workType = "SUB (PV AC ACU)";
      }
    }));
  }

  setSbu() {
    this.http.get("/api/sbu", {responseType: "text"})
    .pipe(tap(response =>{
      if(response.indexOf("ACK") >= 0){
        this.workType = "SBU (PV ACU AC)";
      }
    }));
  }
}
