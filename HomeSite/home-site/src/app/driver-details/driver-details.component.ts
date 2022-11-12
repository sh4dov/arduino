import { Component, OnDestroy, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { HttpClient } from '@angular/common/http';
import { Driver, Light } from '../driver';
import { finalize, Subject, tap } from 'rxjs';
import { RepeaterServiceFactory, RepeateService } from '../services/RepeateService';

@Component({
  selector: 'app-driver-details',
  templateUrl: './driver-details.component.html',
  styleUrls: ['./driver-details.component.css']
})
export class DriverDetailsComponent implements OnInit, OnDestroy {
  private id: number = 0;
  driver: Driver | undefined;
  private repeate: RepeateService | undefined;

  constructor(private route: ActivatedRoute,
    private http: HttpClient,
    private repeateFactory: RepeaterServiceFactory) { }

  ngOnInit(): void {
    const routePrams = this.route.snapshot.paramMap;
    this.id = Number(routePrams.get('id'));

    this.repeate = this.repeateFactory.create(() => this.getDriver());
  }

  ngOnDestroy(){
    this.repeate?.unsubscribe();
  }

  getDriver(){
    this.http.get<Driver>("/api/drivers/" + this.id)
      .pipe(
        tap(driver => this.driver = driver),
        finalize(() => {
            this.repeate?.repeate()
        })
      )
      .subscribe();
  }

  changeState(light: Light){
    this.http.post("http://" + this.driver?.ip + "/onoff", JSON.stringify({ id: light.id, value: !light.on }), { responseType: "text" })
    .pipe(
      tap(() => {
          light.on = !light.on;
      })
    )
    .subscribe();
  }

  changeAuto(light: Light) {
    this.http.post("http://" + this.driver?.ip + "/auto", JSON.stringify({ id: light.id, value: light.auto }), { responseType: "text" })
    .subscribe();
  }

  changeBrightness(light: Light) {
    this.http.post("http://" + this.driver?.ip + "/brightness", JSON.stringify({ id: light.id, value: light.brightness }), { responseType: "text" })
    .subscribe();
  }

  saveSettings() {
    this.http.post("http://" + this.driver?.ip + "/save", JSON.stringify({ from: this.driver?.from, to: this.driver?.to }), { responseType: "text" });
  }
}
