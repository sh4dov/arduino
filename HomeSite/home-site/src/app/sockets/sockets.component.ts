import { HttpClient } from '@angular/common/http';
import { Component, OnInit } from '@angular/core';
import { finalize, tap } from 'rxjs';

interface Socket {
  id: number;
  data: string
}

@Component({
  selector: 'app-sockets',
  templateUrl: './sockets.component.html',
  styleUrls: ['./sockets.component.css']
})
export class SocketsComponent implements OnInit {
  public sockets: Socket[] = [];

  constructor(private http: HttpClient) { }

  ngOnInit(): void {
    this.getSockets();
  }

  getSockets(){
    this.http.get<Socket[]>("/api/sockets")
    .pipe(
      tap(sockets => this.sockets = sockets),
      finalize(() => setTimeout(() => this.getSockets(), 10000))
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
