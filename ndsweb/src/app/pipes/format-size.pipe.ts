import { Pipe, PipeTransform } from '@angular/core';

const BYTES_IN_GIGABYTE = 1073741824;
const BYTES_IN_MEGABYTE = 1048576;
const BYTES_IN_KILOBYTE = 1024;

@Pipe({
    name: 'formatSize',
})
export class FormatSizePipe implements PipeTransform {
    transform(value: string | number): string {
        let bytes: number = value as number;

        if (typeof value === 'string') {
            bytes = parseInt(value);
        }

        const gigabytes = bytes / BYTES_IN_GIGABYTE;
        const megabytes = bytes / BYTES_IN_MEGABYTE;
        const kilobytes = bytes / BYTES_IN_KILOBYTE;

        if (gigabytes > 1) {
            return `${gigabytes.toFixed(2)} GB`;
        }

        if (megabytes > 1) {
            return `${megabytes.toFixed(2)} MB`;
        }

        if (kilobytes > 1) {
            return `${kilobytes.toFixed(2)} KB`;
        }

        return `${bytes} B`;
    }
}
