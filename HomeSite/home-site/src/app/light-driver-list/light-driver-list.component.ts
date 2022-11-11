import { HttpClient } from '@angular/common/http';
import { Component, OnInit, Input } from '@angular/core';
import { Driver, drivers } from '../driver';
import {map, shareReplay, tap} from 'rxjs/operators';

@Component({
  selector: 'app-light-driver-list',
  templateUrl: './light-driver-list.component.html',
  styleUrls: ['./light-driver-list.component.css']
})
export class LightDriverListComponent implements OnInit {
  drivers: Driver[] = drivers;

  constructor() { }

  ngOnInit(): void {
  }
}
