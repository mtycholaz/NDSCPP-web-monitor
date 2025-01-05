import { HttpErrorResponse } from '@angular/common/http';
import { Injectable } from '@angular/core';

import { catchError, finalize, switchMap, take, tap, timer } from 'rxjs';

import {
    Action,
    createSelector,
    State,
    StateContext,
    Store,
} from '@ngxs/store';

import { MonitorActions } from '../actions';
import { Canvas, MonitorService } from '../services/monitor.service';

const REFRESH_INTERVAL_IN_MS = 80;

export interface StateModel {
    autoRefresh: boolean;
    isLoading: boolean;
    canvases: Canvas[];
    connectionError: HttpErrorResponse | null;
}

@State<StateModel>({
    name: 'monitor',
    defaults: {
        autoRefresh: true,
        isLoading: false,
        connectionError: null,
        canvases: [],
    },
})
@Injectable()
export class MonitorState {
    static getCanvases() {
        return createSelector(
            [MonitorState],
            (state: StateModel) => state.canvases
        );
    }

    static autoRefresh() {
        return createSelector(
            [MonitorState],
            (state: StateModel) => state.autoRefresh
        );
    }

    static hasConnectionError() {
        return createSelector(
            [MonitorState],
            (state: StateModel) => state.connectionError !== null
        );
    }

    static connectionError() {
        return createSelector(
            [MonitorState],
            (state: StateModel) => state.connectionError
        );
    }

    constructor(private monitorService: MonitorService, private store: Store) {}

    @Action(MonitorActions.UpdateAutoRefresh)
    updateAutoRefresh(
        { patchState }: StateContext<StateModel>,
        { value }: MonitorActions.UpdateAutoRefresh
    ) {
        patchState({ autoRefresh: value });

        if (value) {
            this.store.dispatch(new MonitorActions.LoadCanvases());
        }
    }

    @Action(MonitorActions.LoadCanvases)
    loadCanvases(
        { patchState, getState, dispatch }: StateContext<StateModel>,
        action: MonitorActions.LoadCanvases
    ) {
        const { isLoading } = getState();

        if (isLoading) {
            return;
        }

        patchState({ isLoading: true });

        return timer(REFRESH_INTERVAL_IN_MS).pipe(
            take(1),
            switchMap(() => this.monitorService.getCanvases()),
            tap((canvases) => {
                patchState({ canvases, connectionError: null });
            }),
            catchError((error) =>
                dispatch(new MonitorActions.LoadCanvasesFailure(error))
            ),
            finalize(() => {
                patchState({ isLoading: false });
                const { autoRefresh } = getState();

                if (autoRefresh) {
                    // Instead of firing off actions on a timer, we check if the user has toggled auto-refresh
                    // and if so, we dispatch a new action to load the controller data.
                    // This is a more reactive approach, and allows the user to control the refresh rate
                    // Also prevents multiple requests from being fired off to the server if the request
                    // takes longer than the interval
                    dispatch(new MonitorActions.LoadCanvases());
                }
            })
        );
    }

    @Action(MonitorActions.ActivateCanvases)
    activateCanvases(
        ctx: StateContext<StateModel>,
        { canvases }: MonitorActions.ActivateCanvases
    ) {
        if (canvases.length === 0) {
            return;
        }

        return this.monitorService.activateCanvases(canvases);
    }

    @Action(MonitorActions.DeactivateCanvases)
    deactivateCanvases(
        ctx: StateContext<StateModel>,
        { canvases }: MonitorActions.DeactivateCanvases
    ) {
        if (canvases.length === 0) {
            return;
        }

        return this.monitorService.deactivateCanvases(canvases);
    }

    @Action([MonitorActions.LoadCanvasesFailure])
    handleError(
        { patchState }: StateContext<StateModel>,
        { error }: MonitorActions.LoadCanvasesFailure
    ) {
        if (error instanceof HttpErrorResponse) {
            patchState({ connectionError: error });
        }
    }
}
