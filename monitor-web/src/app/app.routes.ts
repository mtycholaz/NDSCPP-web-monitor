import {
    ActivatedRouteSnapshot,
    Route,
    RouterStateSnapshot,
} from '@angular/router';

import { MonitorContainerComponent } from './containers';
import { Store } from '@ngxs/store';
import { MonitorActions } from './actions';
import { inject } from '@angular/core';

export const appRoutes: Route[] = [
    {
        path: '',
        component: MonitorContainerComponent,
        resolve: {
            data: (
                _route: ActivatedRouteSnapshot,
                _state: RouterStateSnapshot,
                store: Store = inject(Store)
            ) => {
                store.dispatch(new MonitorActions.LoadCanvases());

            },
        },
    },
    { path: '**', redirectTo: '' },
];
