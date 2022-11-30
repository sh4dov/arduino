import { Component, OnInit } from '@angular/core';
import { DriverStore } from './services/stores/driverStore';
import { ESBStore } from './services/stores/ESBStore';
import { SocketStore } from './services/stores/SocketStore';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {

  constructor(
    private driverStore: DriverStore,
    private esbStore: ESBStore,
    private socketStore: SocketStore) {
  }

  ngOnInit(): void {
    this.driverStore.initialize();
    this.esbStore.initialize();
    this.socketStore.initialize();
  }
}
