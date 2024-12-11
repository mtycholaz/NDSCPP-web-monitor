import { ChangeDetectionStrategy, Component } from '@angular/core';

import { Store } from '@ngxs/store';
import { MonitorState } from 'src/app/state';

import { MonitorActions } from '../../actions';
import { MonitorComponent } from '../../components';

@Component({
    templateUrl: './monitor-container.component.html',
    styleUrls: ['./monitor-container.component.scss'],
    changeDetection: ChangeDetectionStrategy.OnPush,
    imports: [MonitorComponent]
})
export class MonitorContainerComponent {

    canvases = this.store.selectSignal(MonitorState.getCanvases());

    constructor(private store: Store) { }

    onRefresh() {
        this.store.dispatch(new MonitorActions.LoadControllerData());
    }
}