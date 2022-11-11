import { ComponentFixture, TestBed } from '@angular/core/testing';

import { ESBComponent } from './esb.component';

describe('ESBComponent', () => {
  let component: ESBComponent;
  let fixture: ComponentFixture<ESBComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ ESBComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(ESBComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
