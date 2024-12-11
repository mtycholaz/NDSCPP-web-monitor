import { CommonModule, DatePipe } from '@angular/common';
import { Component, input, output, signal } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatCheckboxModule } from '@angular/material/checkbox';
import { MatIconModule } from '@angular/material/icon';
import { MatTableModule } from '@angular/material/table';

import { filter, tap, timer } from 'rxjs';

import { Canvas } from 'src/app/services';

@Component({
    selector: 'app-monitor',
    templateUrl: './monitor.component.html',
    styleUrls: ['./monitor.component.scss'],
    imports: [
        CommonModule,
        MatTableModule,
        MatButtonModule,
        MatCardModule,
        MatIconModule,
        MatCheckboxModule,
        DatePipe
    ],
})
export class MonitorComponent {

    lastRefresh: Date = new Date();
    autoRefresh = true;
    refresh$ = timer(0, 3000).pipe(
        filter(() => this.autoRefresh),
        tap(() => this.refresh.emit()),
        tap(() => this.lastRefresh = new Date())

    ).subscribe();

    canvases = input<Canvas[]>([]);
    refresh = output();

    displayedColumns = ['name', 'feature', 'host', 'fps', 'effect'];

    onRefreshClicked() {
        this.refresh.emit();
        this.lastRefresh = new Date();
    }
}
