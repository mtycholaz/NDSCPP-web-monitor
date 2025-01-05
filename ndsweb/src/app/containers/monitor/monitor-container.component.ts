import { CommonModule } from '@angular/common';
import { ChangeDetectionStrategy, Component } from '@angular/core';
import { MatCardModule } from '@angular/material/card';

import { Store } from '@ngxs/store';
import { MonitorState } from 'src/app/state';

import { MonitorActions } from '../../actions';
import { MonitorComponent } from '../../components';
import { MatIcon } from '@angular/material/icon';

@Component({
    templateUrl: './monitor-container.component.html',
    styleUrls: ['./monitor-container.component.scss'],
    changeDetection: ChangeDetectionStrategy.OnPush,
    imports: [MonitorComponent, MatCardModule, MatIcon, CommonModule],
    standalone: true,
})
export class MonitorContainerComponent {
    canvases = this.store.selectSignal(MonitorState.getCanvases());
    connectionError = this.store.selectSignal(
        MonitorState.connectionError()
    );

    constructor(private store: Store) {}

    onAutoRefresh(value: boolean) {
        this.store.dispatch(new MonitorActions.UpdateAutoRefresh(value));
    }
}
