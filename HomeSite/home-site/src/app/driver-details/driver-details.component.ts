import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { DriverStore } from '../services/stores/driverStore';

@Component({
  selector: 'app-driver-details',
  templateUrl: './driver-details.component.html',
  styleUrls: ['./driver-details.component.css']
})
export class DriverDetailsComponent implements OnInit {
  id: number = 0;

  constructor(private route: ActivatedRoute,
    public driverStore: DriverStore) { }

  ngOnInit(): void {
    const routePrams = this.route.snapshot.paramMap;
    this.id = Number(routePrams.get('id'));
  }
}
