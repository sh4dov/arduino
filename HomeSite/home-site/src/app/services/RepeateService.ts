import { Injectable } from "@angular/core";
import { Unsubscribable } from "rxjs";

@Injectable({
  providedIn: 'root'
})
export class RepeaterServiceFactory {
  create(callback: () => void, timeout: number = 10000): RepeateService{
    return new RepeateService(callback, timeout);
  }
}

export class RepeateService implements Unsubscribable {
  private callback: (() => void) | null = null;
  private timeout: number = 0;

  constructor(callback: () => void, timeout: number = 10000) {
    this.callback = callback;
    this.timeout = timeout;

    if(this.callback){
      this.callback();
    }
  }

  repeate() {
    setTimeout(() => {
      if(this.callback){
        this.callback();
      }
    }, this.timeout);
  }

  unsubscribe(): void {
    this.callback = null;
  }
}
