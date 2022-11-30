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
import { HomeComponent } from './home/home.component';
import { RepeaterServiceFactory, RepeateService } from './services/RepeateService';
import { DriverStore } from './services/stores/driverStore';
import { ESBStore } from './services/stores/ESBStore';
import { SocketStore } from './services/stores/SocketStore';

@NgModule({
  imports: [
    BrowserModule,
    HttpClientModule,
    FormsModule,
    RouterModule.forRoot([
      { path: '', component: HomeComponent },
      { path: 'lights', component: LightDriverListComponent },
      { path: 'lights/:id', component: DriverDetailsComponent },
      { path: 'epg', component: EpgComponent },
      { path: 'esb', component: ESBComponent }
    ], { useHash: true })
  ],
  declarations: [
    AppComponent,
    LightDriverListComponent,
    DriverDetailsComponent,
    ESBComponent,
    SocketsComponent,
    EpgComponent,
    HomeComponent
  ],
  providers: [RepeaterServiceFactory, DriverStore, ESBStore, SocketStore],
  bootstrap: [AppComponent]
})
export class AppModule { }
