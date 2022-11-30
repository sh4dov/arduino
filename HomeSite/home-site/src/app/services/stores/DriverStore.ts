import { HttpClient } from "@angular/common/http";
import { Injectable } from "@angular/core";
import { BehaviorSubject, filter, finalize, interval, map, Observable, switchMap, tap } from "rxjs";
import { Driver, Light } from "src/app/driver";

@Injectable({
  providedIn: 'root'
})
export class DriverStore {
  private subject = new BehaviorSubject<Driver[]>([]);
  private loading = false;

  drivers = this.subject.asObservable();

  constructor(
    private http: HttpClient) {
  }

  initialize() {
    interval(10000)
    .pipe(
      filter(() => !this.loading),
      filter(x => this.subject.observed || !(x % 6) || !this.subject.getValue()),
      tap(() => this.loading = true),
      switchMap(() => this.getDrivers()),
      tap(() => this.loading = false),
      finalize(() => this.loading = false))
    .subscribe();
  }

  getDriver(id: number): Observable<Driver | undefined> {
    return this.drivers
    .pipe(
      map((drivers: Driver[]) => drivers.find(d => d.id == id))
    )
  }

  changeState(driver: Driver, light: Light){
    light.loading = true;
    this.http.post("http://" + driver?.ip + "/onoff", JSON.stringify({ id: light.id, value: !light.on }), { responseType: "text" })
    .pipe(
      tap(() => {
          light.on = !light.on;
      }),
      finalize(() => light.loading = false)
    )
    .subscribe();
  }

  changeAuto(driver: Driver, light: Light) {
    this.http.post("http://" + driver?.ip + "/auto", JSON.stringify({ id: light.id, value: light.auto }), { responseType: "text" })
    .subscribe();
  }

  changeBrightness(driver: Driver, light: Light) {
    this.http.post("http://" + driver?.ip + "/brightness", JSON.stringify({ id: light.id, value: light.brightness }), { responseType: "text" })
    .subscribe();
  }

  saveSettings(driver: Driver) {
    this.http.post("http://" + driver?.ip + "/save", JSON.stringify({ from: driver?.from, to: driver?.to }), { responseType: "text" });
  }

  private getDrivers(): Observable<Driver[]> {
    return this.http.get<Driver[]>("/api/drivers")
      .pipe(
        tap(drivers => this.subject.next(drivers))
      );
  }
}
