import { Component } from '@angular/core';
import { DriverStore } from '../services/stores/driverStore';

@Component({
  selector: 'app-light-driver-list',
  templateUrl: './light-driver-list.component.html',
  styleUrls: ['./light-driver-list.component.css']
})
export class LightDriverListComponent {

  constructor(
    public driverStore: DriverStore) { }
}
