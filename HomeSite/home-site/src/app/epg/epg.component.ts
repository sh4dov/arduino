import { HttpClient } from '@angular/common/http';
import { Component, OnInit } from '@angular/core';
import { BehaviorSubject, finalize, map, tap } from 'rxjs';

interface Record {
  channel: string,
  start: string,
  stop: string,
  title: string,
  categories: string[],
  desc: string,
  img: string,
  channelImg: string,
  startDisplay: string,
  stopDisplay: string,
  no: number
}

@Component({
  selector: 'app-epg',
  templateUrl: './epg.component.html',
  styleUrls: ['./epg.component.css']
})
export class EpgComponent implements OnInit {
  private isLoadingSubject = new BehaviorSubject(false);
  public records: Record[] = [];
  public isLoading = this.isLoadingSubject.asObservable();
  public time = new Date().getHours();

  constructor(private http: HttpClient) { }

  ngOnInit(): void {
    this.getEpg(new Date());
  }

  getEpg(date: Date){
    const query = "" + date.getFullYear() + (date.getMonth() > 8 ? date.getMonth() + 1 : "0" + (date.getMonth() + 1)) + (date.getDate() > 9 ? date.getDate() : "0" + date.getDate()) + (date.getHours() > 9 ? date.getHours() : "0" + date.getHours());
    this.isLoadingSubject.next(true);
    this.records = [];

    this.http.get<Record[]>("/epg/" + query)
    .pipe(
      tap(records => {
      records.forEach(r => {
        const start = new Date(r.start);
        const stop = new Date(r.stop);
        r.startDisplay = `${start.getHours()}:${start.getMinutes() > 9 ? start.getMinutes() : "0" + start.getMinutes()}`;
        r.stopDisplay = `${stop.getHours()}:${stop.getMinutes() > 9 ? stop.getMinutes() : "0" + stop.getMinutes()}`;
      });
      this.records = records;
    }),
    finalize(() => this.isLoadingSubject.next(false)))
    .subscribe();
  }

  timeUp(){
    if(this.time < 23){
      this.time++;
    }
  }

  timeDown(){
    if(this.time > 0){
      this.time--;
    }
  }

  search(){
    const d = new Date();
    d.setHours(this.time);
    this.getEpg(d);
  }

}
