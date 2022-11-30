import { HttpClient } from "@angular/common/http";
import { Injectable } from "@angular/core";
import { BehaviorSubject, concat, connect, filter, finalize, forkJoin, interval, map, merge, of, switchMap, tap, toArray } from "rxjs";

export interface StatsDisplay {
  year: string,
  month: string,
  day: string,
  total: string
}

export interface Params {
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

interface Stats {
  year: number,
  month: number,
  day: number,
  total: number
}

interface Val {
  value: number
}

export interface ValDisplay {
  day: string;
  value: string;
  loading: boolean;
}

@Injectable({
  providedIn: 'root'
})
export class ESBStore {
  private statsSubject = new BehaviorSubject<StatsDisplay>({day: "", month: "", total: "", year: ""});
  private paramsSubject = new BehaviorSubject<Params>({ac: {voltage: "", hz: ""}, ac_out: {voltage: "", hz: ""}, acu: {charging: "", discharge: "", soc: "", voltage: ""},load: "", power: {active: "", apparent: ""}, pv: {amp: "", voltage: "", watt: ""}, temp: "", v_bus: ""});
  private workTypeSubject = new BehaviorSubject<string>("");
  private daysSubject = new BehaviorSubject<ValDisplay[]>([]);
  private monthsSybject = new BehaviorSubject<ValDisplay[]>([]);
  private loading = false;
  private loadingDates = false;
  private loadingMonths = false;

  stats = this.statsSubject.asObservable();
  params = this.paramsSubject.asObservable();
  workType = this.workTypeSubject.asObservable();
  days = this.daysSubject.asObservable();
  months = this.monthsSybject.asObservable();

  constructor(
    private http: HttpClient) {
  }

  initialize() {
    interval(10000)
    .pipe(
      filter(() => !this.loading),
      filter(x => !(x % 6) || this.statsSubject.observed || this.paramsSubject.observed || this.workTypeSubject.observed),
      tap(() => this.loading = true),
      switchMap(() => this.getStats()),
      switchMap(() => this.getParams()),
      switchMap(() => this.getWorkType()),
      tap(() => this.loading = false),
      finalize(() => this.loading = false)
    )
    .subscribe();

    interval(10000)
    .pipe(
      filter(() => !this.loadingDates),
      filter(x => !(x % 60)),
      tap(() => {
        this.loadingDates = true;
        console.log("start loading days");
      }),
      switchMap(() => concat(...[... Array(new Date().getDate()).keys()].map(d => {
        const day = new Date();
        day.setDate(d + 1);
        return this.getDayStatus(day);
      })).pipe(toArray())),
      tap(() => {
        this.loadingDates = false;
        console.log("stop loading days");
      }),
      finalize(() => this.loadingDates = false)
    )
    .subscribe();

    interval(10000)
    .pipe(
      filter(() => !this.loadingMonths),
      filter(x => !(x % 60)),
      tap(() => {
        this.loadingMonths = true;
        console.log("start loading months")
      }),
      switchMap(() => concat(...[... Array(12).keys()].map(m => {
        return this.getMonthStatus(m);
      })).pipe(toArray())),
      tap(() => {
        this.loadingMonths = false;
        console.log("stop loading months");
      }),
      finalize(() => this.loadingMonths = false)
    )
    .subscribe();
  }

  setSub() {
    this.http.get("/api/sub", {responseType: "text"})
    .pipe(tap(response =>{
      if(response.indexOf("ACK") >= 0){
        this.workTypeSubject.next("SUB (PV AC ACU)");
      }
    }));
  }

  setSbu() {
    this.http.get("/api/sbu", {responseType: "text"})
    .pipe(tap(response =>{
      if(response.indexOf("ACK") >= 0){
        this.workTypeSubject.next("SBU (PV ACU AC)");
      }
    }));
  }

  private getStats(){
    return this.http.get<Stats>("/api/stats")
    .pipe(tap(stats => {
      this.statsSubject.next({
        day: this.getStatsString(stats.day),
        month: this.getStatsString(stats.month),
        total: this.getStatsString(stats.total),
        year: this.getStatsString(stats.year)
      });
    }))
  }

  private getParams(){
    return this.http.get<Params>("/api/params")
    .pipe(tap(params => {
      this.paramsSubject.next(params);
    }))
  }

  private getWorkType(){
    return this.http.get("/api/worktype", {responseType: "text"})
    .pipe(
      tap(worktype => {
      if(worktype == "sub"){
        this.workTypeSubject.next("SUB (PV AC ACU)");
      }else if(worktype == "sbu"){
        this.workTypeSubject.next("SBU (PV ACU AC)");
      }
      else{
        this.workTypeSubject.next("Unknown");
      }
    }));
  }

  private getMonthStatus(m: number){
    let result: ValDisplay[] = [... Array(12).keys()].map(d =>{
      const date = new Date();
      date.setMonth(d);
      return {day: date.toLocaleString('pl-pl', {month: 'long'}), value: "", loading: true}});
    if(result.length == this.monthsSybject.getValue().length){
      result = this.monthsSybject.getValue();
      if(m == new Date().getMonth() || result[m].value == "")
      {
        result[m].loading = true;
      }
      else{
        return of<Val>(null as unknown as Val);
      }
    }else {
      this.monthsSybject.next([...result]);
    }

    const today = new Date();
    let q = today.getFullYear() + (m >= 9 ? "" + (m + 1) : "0" + (m + 1));
    return this.http.get<Val>("api/qem/" + q)
    .pipe(
      tap(v => {
        result[m].value = this.getStatsString(v.value);
        result[m].loading = false;
        this.monthsSybject.next([... result]);
      })
    );
  }

  private getDayStatus(d: Date){
    const today = new Date();
    let result: ValDisplay[] = [... Array(today.getDate()).keys()].map(d =>{ return {day: (d+1).toString(), value: "", loading: true}});
    if(result.length == this.daysSubject.getValue().length){
      result = this.daysSubject.getValue();
      if((today.getDate() == d.getDate() && today.getMonth() == d.getMonth() && today.getFullYear() == d.getFullYear()) || result[d.getDate() - 1].value == "")
      {
        result[d.getDate() - 1].loading = true;
      }else {
        return of<Val>(null as unknown as Val);
      }
    }else{
      this.daysSubject.next([...result]);
    }


    let q = d.getFullYear() + (d.getMonth() >= 9 ? "" + (d.getMonth() + 1) : "0" + (d.getMonth() + 1)) + (d.getDate() > 9 ? "" + d.getDate() : "0" + d.getDate());
    return this.http.get<Val>("api/qed/" + q)
    .pipe(
      tap(v => {
        result[d.getDate() - 1].value = this.getStatsString(v.value);
        result[d.getDate() - 1].loading = false;
        this.daysSubject.next([... result]);
      })
    );
  }

  private getStatsString(value: number): string {
    if(!value){
      return "0 W";
    }
    return value > 1000 ? value / 1000 + " kW" : value + " W";
  }
}
