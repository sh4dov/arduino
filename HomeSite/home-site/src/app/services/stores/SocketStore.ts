import { HttpClient } from "@angular/common/http";
import { Injectable } from "@angular/core";
import { BehaviorSubject, filter, finalize, interval, switchMap, tap } from "rxjs";

export interface Socket {
  id: number;
  data: string
}

@Injectable({
    providedIn: 'root'
})
export class SocketStore {
  private subject = new BehaviorSubject<Socket[]>([]);
  private loading = false;

  sockets = this.subject.asObservable();

  constructor(
    private http: HttpClient) {
  }

  initialize() {
    interval(10000)
    .pipe(
      filter(() => !this.loading),
      filter(x => !(x % 6) || this.subject.observed || !this.subject.getValue()),
      tap(() => this.loading = true),
      switchMap(() => this.getSockets()),
      tap(() => this.loading = false),
      finalize(() => this.loading = false)
    )
    .subscribe();
  }

  private getSockets(){
    return this.http.get<Socket[]>("/api/sockets")
    .pipe(
      tap(sockets => this.subject.next(sockets))
    );
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
