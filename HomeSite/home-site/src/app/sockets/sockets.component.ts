import { HttpClient } from '@angular/common/http';
import { Component, OnDestroy, OnInit } from '@angular/core';
import { finalize, tap } from 'rxjs';
import { RepeateService } from '../services/RepeateService';

interface Socket {
  id: number;
  data: string
}

@Component({
  selector: 'app-sockets',
  templateUrl: './sockets.component.html',
  styleUrls: ['./sockets.component.css']
})
export class SocketsComponent implements OnInit, OnDestroy {
  public sockets: Socket[] = [];

  constructor(
    private http: HttpClient,
    private repeate: RepeateService) { }

  ngOnDestroy(): void {
    this.repeate.unsubscribe();
  }

  ngOnInit(): void {
    this.repeate.subscribe(() => this.getSockets());
  }

  getSockets(){
    this.http.get<Socket[]>("/api/sockets")
    .pipe(
      tap(sockets => this.sockets = sockets),
      finalize(() => this.repeate.repeate())
    )
    .subscribe();
  }

  turnOn(id: number){
    this.http.get("/api/sockets/" + id + "/on", {responseType: "text"})
    .subscribe();
  }

  turnOff(id: number){
    this.http.get("/api/sockets/" + id + "/off", {responseType: "text"})
    .subscribe();
  }
}
