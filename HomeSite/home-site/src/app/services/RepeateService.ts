import { Injectable } from "@angular/core";
import { Unsubscribable } from "rxjs";

@Injectable()
export class RepeateService implements Unsubscribable {
  private callback: (() => void) | null = null;
  private timeout: number = 0;

  subscribe(callback: () => void, timeout: number = 10000) {
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
