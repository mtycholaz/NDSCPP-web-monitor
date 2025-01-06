import { HttpErrorResponse } from '@angular/common/http';
import { inject, Injectable } from '@angular/core';

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
import { ToastrService } from 'ngx-toastr';
import { MatDialog } from '@angular/material/dialog';
import { ConfirmDialogComponent } from '../dialogs/confirm-dialog.component';

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

    monitorService = inject(MonitorService);
    store = inject(Store);
    toastr = inject(ToastrService);
    dialog = inject(MatDialog);

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

    @Action(MonitorActions.DeleteCanvas)
    deleteCanvas(
        { dispatch }: StateContext<StateModel>,
        { canvas }: MonitorActions.DeleteCanvas
    ) {
        return this.monitorService.deleteCanvas(canvas.id).pipe(
            tap(() => {
                this.toastr.success('Canvas deleted successfully');
            }),
            catchError((error) =>
                dispatch(new MonitorActions.DeleteCanvasFailure(error))
            )
        );
    }

    @Action(MonitorActions.DeleteFeature)
    deleteFeature(
        { dispatch }: StateContext<StateModel>,
        { canvas, feature }: MonitorActions.DeleteFeature
    ) {
        return this.monitorService.deleteFeature(canvas.id, feature.id).pipe(
            tap(() => {
                this.toastr.success('Feature deleted successfully');
            }),
            catchError((error) =>
                dispatch(new MonitorActions.DeleteCanvasFailure(error))
            )
        );
    }

    @Action(MonitorActions.ConfirmDeleteCanvas)
    confirmDeleteCanvas(
        { dispatch }: StateContext<StateModel>,
        { canvas }: MonitorActions.ConfirmDeleteCanvas
    ) {
        return this.dialog
            .open(ConfirmDialogComponent, {
                disableClose: true,
                data: {
                    title: 'Confirm Delete',
                    message: 'Are you certain you want to delete this canvas?',
                    cancelText: 'No',
                    confirmText: 'Yes',
                    confirmIcon: 'delete',
                    confirmClass: 'warn'
                },
            })
            .afterClosed()
            .pipe(
                tap((result) => {
                    if (result) {
                        dispatch(new MonitorActions.DeleteCanvas(canvas));
                    }
                })
            );
    }

    @Action(MonitorActions.ConfirmDeleteFeature)
    confirmDeleteFeature(
        { dispatch }: StateContext<StateModel>,
        { model }: MonitorActions.ConfirmDeleteFeature
    ) {
        return this.dialog
            .open(ConfirmDialogComponent, {
                disableClose: true,
                data: {
                    title: 'Confirm Delete',
                    message: 'Are you certain you want to delete this feature?',
                    cancelText: 'No',
                    confirmText: 'Yes',
                    confirmIcon: 'delete',
                    confirmClass: 'warn'
                },
            })
            .afterClosed()
            .pipe(
                tap((result) => {
                    if (result) {
                        dispatch(
                            new MonitorActions.DeleteFeature(
                                model.canvas,
                                model.feature
                            )
                        );
                    }
                })
            );
    }

    @Action([MonitorActions.LoadCanvasesFailure])
    handleError(
        { patchState }: StateContext<StateModel>,
        { error }: MonitorActions.LoadCanvasesFailure
    ) {
        switch (true) {
            case error instanceof HttpErrorResponse:
                patchState({ connectionError: error });
                break;
            case error instanceof MonitorActions.DeleteCanvasFailure:
                this.toastr.error(error.error.message, 'Error deleting canvas');
                break;
            case error instanceof MonitorActions.DeleteFeatureFailure:
                this.toastr.error(
                    error.error.message,
                    'Error deleting feature'
                );
                break;
        }
    }
}
