import { Component } from '@angular/core';
import { ESBStore } from '../services/stores/ESBStore';

@Component({
  selector: 'app-esb',
  templateUrl: './esb.component.html',
  styleUrls: ['./esb.component.css']
})
export class ESBComponent {
  constructor(
    public store: ESBStore) { }
}
