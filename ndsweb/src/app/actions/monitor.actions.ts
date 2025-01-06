import { Canvas, Feature } from '../services';

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

export class ActivateCanvasesFailure {
    static readonly type = `${ns} Activate Canvases Failure`;
    constructor(public error: Error) {}
}

export class DeactivateCanvases {
    static readonly type = `${ns} Deactivate Canvases`;
    constructor(public canvases: Canvas[]) {}
}

export class DeactivateCanvasesFailure {
    static readonly type = `${ns} Deactivate Canvases Failure`;
    constructor(public error: Error) {}
}

export class DeleteCanvas {
    static readonly type = `${ns} Delete Canvas`;
    constructor(public canvas: Canvas) {}
}

export class DeleteCanvasFailure {
    static readonly type = `${ns} Delete Canvas Failure`;
    constructor(public error: Error) {}
}

export class DeleteFeature {
    static readonly type = `${ns} Delete Feature`;
    constructor(public canvas: Canvas, public feature: Feature) {}
}

export class DeleteFeatureFailure {
    static readonly type = `${ns} Delete Feature Failure`;
    constructor(public error: Error) {}
}

export class ConfirmDeleteCanvas {
    static readonly type = `${ns} Confirm Delete Canvas`;
    constructor(public canvas: Canvas) {}
}

export class ConfirmDeleteFeature {
    static readonly type = `${ns} Confirm Delete Feature`;
    constructor(public model: { canvas: Canvas; feature: Feature }) {}
}
