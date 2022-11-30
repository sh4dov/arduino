import { Component } from '@angular/core';
import { SocketStore } from '../services/stores/SocketStore';

@Component({
  selector: 'app-sockets',
  templateUrl: './sockets.component.html',
  styleUrls: ['./sockets.component.css']
})
export class SocketsComponent {
  constructor(public store: SocketStore) { }
}
