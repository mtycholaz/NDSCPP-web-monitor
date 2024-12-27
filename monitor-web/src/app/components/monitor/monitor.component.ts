import { CommonModule, NgTemplateOutlet } from '@angular/common';
import { Component, HostListener, input, OnChanges, output, SimpleChanges } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
import { MatCheckboxModule } from '@angular/material/checkbox';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatSelectChange, MatSelectModule } from '@angular/material/select';
import { MatTableDataSource, MatTableModule } from '@angular/material/table';

import { filter } from 'rxjs';

import { FormatDeltaPipe, FormatSizePipe } from '../../pipes';
import { Canvas, Feature } from '../../services';

interface RowData {
    isConnected: boolean;
    canvasName: string;
    featureName: string;
    hostName: string;
    size: string;
    reconnectCount: number | null;
    reconnectStatus: 'good' | 'warning' | 'danger' | '' | null;
    canvasFps: number | null;
    featureFps: number | null;
    fpsStatus: 'good' | 'warning' | '' | null;
    queueDepth: number | null;
    queueMaxSize: number | null;
    queueStatus: 'good' | 'warning' | 'danger' | '' | null;
    bufferSize: number | null;
    bufferPosition: number | null;
    bufferStatus: 'good' | 'warning' | 'danger' | '' | null;
    wifiSignal: string | null;
    wifiSignalStatus: 'good' | 'warning' | 'danger' | '' | null;
    bandwidth: number | null;
    currentTime: number | null;
    delta: number | null;
    deltaStatus: 'good' | 'warning' | 'danger' | '' | null;
    flashVersion: string | null;
    status: 'connected' | 'disconnected';
    currentEffectName: string;
    index: string;
}

@Component({
    selector: 'app-monitor',
    templateUrl: './monitor.component.html',
    styleUrls: ['./monitor.component.scss'],
    imports: [
        CommonModule,
        NgTemplateOutlet,
        MatTableModule,
        MatButtonModule,
        MatIconModule,
        MatCheckboxModule,
        MatFormFieldModule,
        MatInputModule,
        FormsModule,
        MatSelectModule,
        FormatSizePipe,
        FormatDeltaPipe,
    ],
    standalone: true,
})
export class MonitorComponent implements OnChanges {
    filter = '';
    sortColumn: string | null = null;
    sortDirection: 'asc' | 'desc' | '' = '';

    dataSource = new MatTableDataSource([] as RowData[]);

    canvases = input<Canvas[]>([]);
    autoRefresh = output<boolean>();

    _displayedColumns = [
        'canvasName',
        'featureName',
        'host',
        'size',
        'reconnectCount',
        'fps',
        'queueDepth',
        'buffer',
        'signal',
        'dataRate',
        'delta',
        'flash',
        'status',
        'effect',
    ];

    columns: { key: string; value: string }[] = [
        { key: 'Canvas', value: 'canvasName' },
        { key: 'Feature', value: 'featureName' },
        { key: 'Host', value: 'host' },
        { key: 'Size', value: 'size' },
        { key: 'Cx', value: 'reconnectCount' },
        { key: 'FPS', value: 'fps' },
        { key: 'Queue', value: 'queueDepth' },
        { key: 'Buffer', value: 'buffer' },
        { key: 'Signal', value: 'signal' },
        { key: 'Data', value: 'dataRate' },
        { key: 'Delta', value: 'delta' },
        { key: 'Flash', value: 'flash' },
        { key: 'Status', value: 'status' },
        { key: 'Effect', value: 'effect' },
    ];

    get displayedColumns(): string[] {
        return this._displayedColumns;
    }

    set displayedColumns(value: string[]) {
        this._displayedColumns = value;
    }

    constructor() {
        this.loadFilterFromStorage();
    }

    onApplyFilter() {
        this.dataSource.filter = this.filter.toLocaleLowerCase();
        this.saveFilterToStorage();
    }

    onSort(column: string) {
        if (this.sortColumn === column) {
            switch (this.sortDirection) {
                case 'asc':
                    this.sortDirection = 'desc';
                    break;
                case 'desc':
                    this.sortColumn = null;
                    this.sortDirection = '';
                    break;
                default:
                    this.sortDirection = 'asc';
                    break;
            }
        } else {
            this.sortColumn = column;
            this.sortDirection = 'asc';
        }

        this.saveFilterToStorage();
    }

    onClearFilter(): void {
        this.filter = '';
        this.dataSource.filter = '';
    }

    updateFilter(filter: string | null) {
        this.filter = filter || '';
        this.dataSource.filter = filter || '';
    }

    trackByFn(index: number, item: RowData) {
        return item.hostName;
    }

    updateDisplayedColumns(event: MatSelectChange) {
        this._displayedColumns = event.value || [];
        this.saveFilterToStorage();
    }

    loadFilterFromStorage() {
        const filterOptions = localStorage.getItem('displayedColumns');
        if (filterOptions) {
            try {
                let { columns, filter, sortColumn, sortDirection } = JSON.parse(filterOptions);
                columns = columns || this._displayedColumns;
                filter = filter || '';
                sortColumn = sortColumn || null;
                sortDirection = sortDirection || '';

                this.sortColumn = sortColumn;
                this.sortDirection = sortDirection;

                // only set the columns if they are valid
                this._displayedColumns = this.columns
                    .filter((c) => columns.includes(c.value))
                    .map((c) => c.value);
                this.filter = filter;
            } catch (e) {}
        }
    }

    saveFilterToStorage() {
        localStorage.setItem(
            'displayedColumns',
            JSON.stringify({
                columns: this._displayedColumns,
                filter: this.filter,
                sortColumn: this.sortColumn,
                sortDirection: this.sortDirection,
            })
        );
    }

    transformFeatureToRowData(canvas: Canvas, feature: Feature) {
        const now = new Date().getTime();
        const isConnected = feature.isConnected;
        const reconnectCount =
            feature.reconnectCount !== undefined
                ? feature.reconnectCount
                : null;

        const data = {
            isConnected,
            canvasName: canvas.name,
            featureName: feature.friendlyName,
            hostName: feature.hostName,
            size: `${feature.height}x${feature.width}`,
            reconnectCount: reconnectCount,
            reconnectStatus: !reconnectCount
                ? ''
                : reconnectCount < 3
                ? 'good'
                : reconnectCount < 10
                ? 'warning'
                : 'danger',
            canvasFps: canvas.fps,
            featureFps: 90,
            fpsStatus: null,
            bufferSize: null,
            bufferPosition: null,
            wifiSignal: null,
            bandwidth: null,
            currentTime: null,
            flashVersion: null,
            status: 'disconnected',
            currentEffectName: canvas.currentEffectName || null,
        } as RowData;

        if (isConnected && feature.lastClientResponse) {
            const { lastClientResponse } = feature;

            const fps = lastClientResponse.fpsDrawing;
            const bufferPos = lastClientResponse.bufferPos;
            const bufferSize = lastClientResponse.bufferSize;
            const signal = Math.abs(lastClientResponse.wifiSignal);
            const bytesPerSecond = feature.bytesPerSecond;
            const currentTime = lastClientResponse.currentClock;
            const queueDepth = feature.queueDepth;
            const queueMaxSize = feature.queueMaxSize;
            const flashVersion =
                lastClientResponse.flashVersion !== undefined
                    ? lastClientResponse.flashVersion.toString()
                    : null;
            const ratio = bufferPos / bufferSize;

            data.featureFps = fps;
            data.bufferSize = bufferSize;
            data.bufferPosition = bufferPos;
            data.wifiSignal = signal >= 100 ? 'LAN' : `${~~signal} dBm`;
            data.bandwidth = bytesPerSecond;
            data.currentTime = currentTime;
            data.delta = currentTime - now;
            data.flashVersion = flashVersion;
            data.status = 'connected';
            data.queueDepth = queueDepth;
            data.queueMaxSize = queueMaxSize;
            data.queueStatus =
                queueDepth < 100
                    ? 'good'
                    : queueDepth < 250
                    ? 'warning'
                    : 'danger';

            data.fpsStatus = fps < 0.8 * canvas.fps ? 'warning' : 'good';
            data.bufferStatus =
                ratio >= 0.25 && ratio <= 0.85
                    ? 'good'
                    : ratio > 0.95
                    ? 'warning'
                    : 'danger';

            data.wifiSignalStatus =
                signal >= 100
                    ? ''
                    : signal < 70
                    ? 'good'
                    : signal < 80
                    ? 'warning'
                    : 'danger';
        }

        const index = [
            (data.hostName || '').toLocaleLowerCase(),
            (data.featureName || '').toLocaleLowerCase(),
            (data.canvasName || '').toLocaleLowerCase(),
            (data.currentEffectName || '').toLocaleLowerCase(),
        ];

        data.index = index.join(';');

        return data;
    }

    // listen for the control key and refresh the data when it is pressed
    @HostListener('document:keydown.control')
    onControlDown() {
        this.autoRefresh.emit(false);
    }

    @HostListener('document:keyup.control')
    onControlUp() {
        this.autoRefresh.emit(true);
    }

    update() {
        const data = this.canvases().reduce((acc, canvas) => {
            const features = canvas.features.map((feature) =>
                this.transformFeatureToRowData(canvas, feature)
            );

            acc.push(...features);

            return acc;
        }, [] as RowData[]);

        if (this.sortColumn) {
            data.sort((a: any, b: any) => {
                if (!this.sortColumn) {
                    return 0;
                }
                const aVal = (a[this.sortColumn] || '').toString().toLocaleLowerCase();
                const bVal = (b[this.sortColumn] || '').toString().toLocaleLowerCase();

                if (aVal === bVal) {
                    return 0;
                }

                return this.sortDirection === 'asc' ? aVal.localeCompare(bVal) : bVal.localeCompare(aVal);
            });
        }

        this.dataSource = new MatTableDataSource(data);

        this.dataSource.filterPredicate = this.onFilterRowData.bind(this);
        this.dataSource.filter = this.filter + '.';
    }

    onFilterRowData(data: RowData, filter: string): boolean {
        return data.index.includes(this.filter.toLocaleLowerCase());
    }

    ngOnChanges(changes: SimpleChanges): void {
        if (changes['canvases']) {
            this.update();
        }
    }
}
