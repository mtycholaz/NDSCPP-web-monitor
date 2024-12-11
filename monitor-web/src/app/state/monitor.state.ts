import { Action, createSelector, State, StateContext } from "@ngxs/store";
import { Canvas, MonitorService } from "../services/monitor.service";
import { MonitorActions } from "../actions";
import { tap } from "rxjs";
import { Injectable } from "@angular/core";

export interface StateModel {
    isLoading: boolean;
    canvases: Canvas[];
}

@State<StateModel>({
    name: 'monitor',
    defaults: {
        isLoading: false,
        canvases: []
    }
})
@Injectable()
export class MonitorState {

    static getCanvases() {
        return createSelector([MonitorState], (state: StateModel) => state.canvases);
    }

    constructor(private monitorService: MonitorService) { }

    @Action(MonitorActions.LoadControllerData)
    loadControllerData(ctx: StateContext<StateModel>, action: MonitorActions.LoadControllerData) {
        return this.monitorService.getCanvases().pipe(
            tap((canvases) => {
                ctx.patchState({ canvases });
            })
        )
    }
}