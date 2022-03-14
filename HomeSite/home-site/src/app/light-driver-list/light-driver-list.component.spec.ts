import { ComponentFixture, TestBed } from '@angular/core/testing';

import { LightDriverListComponent } from './light-driver-list.component';

describe('LightDriverListComponent', () => {
  let component: LightDriverListComponent;
  let fixture: ComponentFixture<LightDriverListComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ LightDriverListComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(LightDriverListComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
