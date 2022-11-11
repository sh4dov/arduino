import { HttpClient } from '@angular/common/http';
import { Component, OnInit, Input, OnDestroy } from '@angular/core';
import { Driver } from '../driver';
import { finalize, tap } from 'rxjs/operators';
import { RepeateService } from '../services/RepeateService';

@Component({
  selector: 'app-light-driver-list',
  templateUrl: './light-driver-list.component.html',
  styleUrls: ['./light-driver-list.component.css']
})
export class LightDriverListComponent implements OnInit, OnDestroy {
  drivers: Driver[] = [];

  constructor(
    private http: HttpClient,
    private repeate: RepeateService) { }

  ngOnDestroy(): void {
    this.repeate.unsubscribe();
  }

  ngOnInit(): void {
    this.repeate.subscribe(() => this.getDrivers());
  }

  getDrivers() {
    this.http.get<Driver[]>("/api/drivers")
    .pipe(tap(drivers => {
      this.drivers = drivers;
    }),
    finalize(() => this.repeate.repeate()))
    .subscribe();
  }
}
