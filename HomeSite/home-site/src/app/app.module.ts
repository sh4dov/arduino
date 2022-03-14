import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { RouterModule } from '@angular/router';
import { HttpClientModule } from '@angular/common/http';
import { AppComponent } from './app.component';
import { LightDriverListComponent } from './light-driver-list/light-driver-list.component';
import { DriverDetailsComponent } from './driver-details/driver-details.component';
import { FormsModule } from '@angular/forms';

@NgModule({
  imports: [
    BrowserModule,
    HttpClientModule,
    FormsModule,
    RouterModule.forRoot([
      { path: '', component: LightDriverListComponent },
      { path: 'drivers/:id', component: DriverDetailsComponent }
    ])
  ],
  declarations: [
    AppComponent,
    LightDriverListComponent,
    DriverDetailsComponent
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
