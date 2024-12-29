import { Pipe, PipeTransform } from '@angular/core';

@Pipe({
    name: 'formatDelta',
})
export class FormatDeltaPipe implements PipeTransform {
    transform(value: string | number, threshold = 5, width = 21): string {
        let time: number = value as number;

        if (typeof value === 'string') {
            time = parseFloat(value);
        }

        if (Math.abs(time) > 100) {
            return 'Unset';
        }

        if (width % 2 == 0) {
            width++;
        }

        // Normalize value to -1..1 range based on threshold
        const normalized = Math.min(1.0, Math.max(-1.0, time / threshold));

        const center = ~~(width / 2);
        const pos = ~~(center + normalized * center);

        let meter = '';
        for (let i = 0; i < width; i++) {
            meter += i == pos ? '|' : '-';
        }
        return `${time.toFixed(1)}s ${meter}`;
    }
}
