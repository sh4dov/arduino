import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { RouterModule } from '@angular/router';
import { HttpClientModule } from '@angular/common/http';
import { AppComponent } from './app.component';
import { LightDriverListComponent } from './light-driver-list/light-driver-list.component';
import { DriverDetailsComponent } from './driver-details/driver-details.component';
import { FormsModule } from '@angular/forms';
import { ESBComponent } from './esb/esb.component';
import { SocketsComponent } from './sockets/sockets.component';
import { EpgComponent } from './epg/epg.component';

@NgModule({
  imports: [
    BrowserModule,
    HttpClientModule,
    FormsModule,
    RouterModule.forRoot([
      { path: '', component: LightDriverListComponent },
      { path: 'drivers/:id', component: DriverDetailsComponent },
      { path: 'epg', component: EpgComponent }
    ], { useHash: true })
  ],
  declarations: [
    AppComponent,
    LightDriverListComponent,
    DriverDetailsComponent,
    ESBComponent,
    SocketsComponent,
    EpgComponent
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
