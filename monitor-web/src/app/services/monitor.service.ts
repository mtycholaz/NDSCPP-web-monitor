import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';

@Injectable()
export class MonitorService {
    constructor(private http: HttpClient) {}

    getCanvases() {
        return this.http.get<ControllerResponse>(
            'http://localhost:7777/api/controller'
        );
    }
}

interface ControllerResponse {
    controller: {
        canvases: Canvas[];
    };
}

export interface Canvas {
    currentEffectName: string;
    effectsManager: {
        currentEffectIndex: number;
        effects: Effect[];
        fps: number;
        type: 'EffectsManager';
    };
    features: Feature[];
    fps: number;
    height: number;
    id: number;
    name: string;
    width: number;
}

interface Feature {
    bytesPerSecond: number;
    channel: number;
    clientBufferCount: number;
    friendlyName: string;
    height: number;
    hostName: string;
    id: number;
    isConnected: boolean;
    lastClientResponse: {
        brightness: number;
        bufferPos: number;
        bufferSize: number;
        currentClock: number;
        flashVersion: number;
        fpsDrawing: number;
        newestPacket: number;
        oldestPacket: number;
        responseSize: number;
        sequenceNumber: number;
        watts: number;
        wifiSignal: number;
    };
    offsetX: number;
    offsetY: number;
    port: number;
    queueDepth: number;
    queueMaxSize: number;
    reconnectCount: number;
    redGreenSwap: boolean;
    reversed: boolean;
    timeOffset: number;
    type: string;
    width: number;
}

interface Effect {
    maxSpeed: number;
    name: string;
    newParticleProbability: number;
    particleFadeTime: number;
    particleHoldTime: number;
    particleIgnition: number;
    particlePreignitionTime: number;
    particleSize: number;
    type: string;

    brightness: number;
    density: number;
    dotSize: number;
    everyNthDot: number;
    ledColorPerSecond: number;
    ledScrollSpeed: number;
    mirrored: boolean;
    palette: {
        blend: boolean;
        colors: {
            b: number;
            g: number;
            r: number;
        }[];
    };
    rampedColor: false;
}
