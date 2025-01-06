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