import { Canvas } from '../services';

const ns = '[Monitor]';

export class UpdateAutoRefresh {
    static readonly type = `${ns} Update Auto Refresh`;
    constructor(public value: boolean) {}
}

export class LoadCanvases {
    static readonly type = `${ns} Load Canvases`;
}

export class LoadCanvasesFailure {
    static readonly type = `${ns} Load Canvases Failure`;
    constructor(public error: any) {}
}

export class ActivateCanvases {
    static readonly type = `${ns} Activate Canvases`;
    constructor(public canvases: Canvas[]) {}
}

export class DeactivateCanvases {
    static readonly type = `${ns} Deactivate Canvases`;
    constructor(public canvases: Canvas[]) {}
}
