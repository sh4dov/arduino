import { ComponentFixture, TestBed } from '@angular/core/testing';

import { SocketsComponent } from './sockets.component';

describe('SocketsComponent', () => {
  let component: SocketsComponent;
  let fixture: ComponentFixture<SocketsComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ SocketsComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(SocketsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
