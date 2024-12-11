const ns = '[Monitor]';

export class LoadControllerData {
    static readonly type = `${ns} Load Controller Data`;
}

export class LoadControllerDataFailure {
    static readonly type = `${ns} Load Controller Data Failure`;
    constructor(public error: any) {}
}